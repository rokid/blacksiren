#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include <cstdio>
#include <error.h>
#include <string.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

#include "sutils.h"
#include "siren.h"
#include "isiren.h"
#include "siren_base.h"
#include "siren_proxy.h"
#include "siren_config.h"

namespace BlackSiren {

RecordingThread::RecordingThread(SirenProxy *siren) :
    pSiren(siren),
    recordingStart(false),
    recordingTerm(false) {
    SirenConfig config = pSiren->global_config->getConfigFile();
    int channels = config.mic_channel_num;
    int sample = config.mic_sample_rate;
    int byte = config.mic_audio_byte;
    int frameLenInMs = config.mic_frame_length;

    frameSize = channels * sample * byte * 1000 / frameLenInMs;
    siren_printf(SIREN_INFO, "recording thread set frame with %d", frameSize);
    frameBuffer = (char *)malloc(frameSize);

    currentRetry = 0;
    errorRetry = config.siren_input_err_retry_num;
    retryTimeout = config.siren_input_err_retry_timeout;
}

RecordingThread::~RecordingThread() {
    if (frameBuffer != nullptr) {
        free(frameBuffer);
    }
}

bool RecordingThread::init() {
    SirenConfig config = pSiren->global_config->getConfigFile();
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        siren_printf(SIREN_ERROR, "init socket pair failed since %s", strerror(errno));
        return false;
    }


    int rmem = config.siren_recording_socket_rmem;
    int wmem = config.siren_recording_socket_wmem;

    if (wmem != 0 && rmem != 0) {
        setsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &wmem, sizeof(wmem));
        setsockopt(sockets[1], SOL_SOCKET, SO_SNDBUF, &wmem, sizeof(wmem));
        setsockopt(sockets[0], SOL_SOCKET, SO_RCVBUF, &rmem, sizeof(rmem));
        setsockopt(sockets[1], SOL_SOCKET, SO_RCVBUF, &rmem, sizeof(rmem));
    }

    int real_rmem = 0;
    int real_wmem = 0;
    socklen_t len = sizeof(int);

    getsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &real_wmem, &len);
    siren_printf(SIREN_INFO, "recording thread get sockets[0] wmem to %d", real_wmem);
    getsockopt(sockets[1], SOL_SOCKET, SO_SNDBUF, &real_wmem, &len);
    siren_printf(SIREN_INFO, "recording thread get sockets[1] wmem to %d", real_wmem);
    getsockopt(sockets[0], SOL_SOCKET, SO_RCVBUF, &real_rmem, &len);
    siren_printf(SIREN_INFO, "recording thread get sockets[0] rmem to %d", real_rmem);
    getsockopt(sockets[1], SOL_SOCKET, SO_RCVBUF, &real_rmem, &len);
    siren_printf(SIREN_INFO, "recording thread get sockets[1] rmem to %d", real_rmem);

    //set non block to writer(sockets[0])
    int opts;
    opts = fcntl(sockets[0], F_GETFL);
    opts = opts | O_NONBLOCK;
    if (fcntl(sockets[0], F_SETFL, opts) < 0) {
        siren_printf(SIREN_ERROR, "proxy recording thread set writer socket to nonblock failed");
    }

    return true;
}

void RecordingThread::start() {
    siren_printf(SIREN_INFO, "start recording thread");
    pSiren->input_callback->start_input(pSiren->token);

    {
        std::unique_lock<decltype(startMutex)> lock(startMutex);
        recordingStart = true;
        startCond.notify_one();
    }
}

void RecordingThread::pause() {
    siren_printf(SIREN_INFO, "stop recording thread");
    {
        std::unique_lock<decltype(startMutex)> lock(startMutex);
        pSiren->input_callback->stop_input(pSiren->token);
        recordingStart = false;
    }
}

void RecordingThread::stop() {
    siren_printf(SIREN_INFO, "terminal recording thread");
    std::unique_lock<decltype(termMutex)> lock(termMutex);
    //let recording start turn true
    recordingTerm = true;
    if (!recordingStart) {
        recordingStart = true;
        startCond.notify_one();
    }
    siren_printf(SIREN_INFO, "waiting recording thread exit");
    if (thread.joinable()) {
        thread.join();
    }
}

void RecordingThread::recordingFn() {
    while (1) {
        int len = 0;
        {
            std::unique_lock<decltype(termMutex)> lock(termMutex);
            if (recordingTerm) {
                siren_printf(SIREN_INFO, "proxy recording thread exit...");
                break;
            }
        }

        {
            std::unique_lock<decltype(startMutex)> lock(startMutex);
            startCond.wait(lock, [this] {
                return recordingStart;
            });
            
            if (recordingTerm) {
                siren_printf(SIREN_INFO, "proxy recording thread exit...");
                return;
            }

            len = pSiren->input_callback->read_input(pSiren->token, frameBuffer, frameSize);
        }

        if (len != 0) {
            siren_printf(SIREN_ERROR, "read input return %d", len);
            if (currentRetry >= errorRetry) {
                siren_printf(SIREN_INFO, "input error retry max");
                pSiren->input_callback->on_err_input(pSiren->token);
            }
            pSiren->input_callback->stop_input(pSiren->token);
            std::this_thread::sleep_for(std::chrono::microseconds(retryTimeout));
            pSiren->input_callback->start_input(pSiren->token);
            continue;
        } else {
            currentRetry = 0;
        }

        //send to other side
        len = write(sockets[0], frameBuffer, frameSize);
        siren_printf(SIREN_INFO, "recording write return %d", len);
        if (len < 0) {
            siren_printf(SIREN_ERROR, "write error on socket with %s", strerror(errno));
            pause();
            continue;
        }
    }
}

siren_status_t SirenProxy::init_siren(void *token, const char *path, siren_input_if_t *input) {
    global_config = new SirenConfigurationManager(path);
    if (global_config == nullptr) {
        siren_printf(SIREN_ERROR, "alloc config manager failed");
        return SIREN_STATUS_ERROR;
    }

    siren_status_t result = SIREN_STATUS_OK;
    result = global_config->parseConfigFile();
    SirenConfig& config = global_config->getConfigFile();
    input_callback = input;

    //use share mem
    if (config.siren_use_share_mem) {

        //use socket
    }

    //init request channel
    if (!requestChannel.open()) {
        siren_printf(SIREN_ERROR, "request channel open failed");
        return SIREN_STATUS_ERROR;
    }
    if (!responseChannel.open()) {
        siren_printf(SIREN_ERROR, "response channel open failed");
        return SIREN_STATUS_ERROR;
    }
    //will release in destroy
    recordingThread = new RecordingThread(this);
    if (!recordingThread->init()) {
        siren_printf(SIREN_ERROR, "init recording thread failed");
        clearThread();
        return SIREN_STATUS_ERROR;
    }

    waitingInit = false;
    set_sig_child_handler();
    //fork true siren
    siren_pid = fork();
    if (siren_pid < 0) {
        siren_printf(SIREN_ERROR, "fork siren failed...");
        delete recordingThread;
        return SIREN_STATUS_ERROR;
    } else if (siren_pid == 0) {
        //gose for siren
        unset_sig_child_handler();
        
        //reader for reponse
        SirenSocketReader reader(&requestChannel);
        SirenSocketWriter writer(&responseChannel);
        
        reader.prepareOnReadSideProcess();
        writer.prepareOnWriteSideProcess();
        
        int socket = recordingThread->getReader();
        SirenBase base(config, socket, reader, writer);
        base.init_siren(nullptr, nullptr, nullptr);
        siren_printf(SIREN_ERROR, "siren exit..");
        exit(0);
    //in parent
    } else {
        launchRequestThread();
        launchResponseThread();

        siren_printf(SIREN_INFO, "waiting response thread...");
        waitingRequestResponseThread();
        siren_printf(SIREN_INFO, "response thread init done");

        //detach recording thread
        std::thread t(&RecordingThread::recordingFn, recordingThread);
        recordingThread->setThread(t);
        recordingThread->getThread().detach();

        siren_printf(SIREN_INFO, "waiting siren base init");
        //waiting response from siren base
        {
            std::unique_lock<decltype(initMutex)> l_(initMutex);
            initCond.wait(l_, [this] {
                return waitingInit;
            });

            siren_printf(SIREN_INFO, "siren base init done");
            if (sirenBaseInitFailed) {
                stopRequestThread(true);
                //waiting response thread exit;
                siren_printf(SIREN_INFO, "waiting request thread exit");
                if (responseThread.joinable()) {
                    responseThread.join();
                }

                siren_printf(SIREN_INFO, "waiting recording thread exit");
                recordingThread->stop();
            }
        }
        siren_printf(SIREN_INFO, "siren init done");
        allocated_from_thread = true;
    }
    return result;
}

void SirenProxy::requestThreadHandler() {
    SirenSocketWriter requestWriter(&requestChannel);
    requestWriter.prepareOnWriteSideProcess();
    while (1) {
        if (requestThreadStop.load(std::memory_order_acquire)) {
            siren_printf(SIREN_INFO, "proxy request thread exit");
            break;
        }

        Request *req;
        int status = requestQueue.pop((void **)&req, nullptr);
        if (status != 0) {
            if (status == -2) {
                siren_printf(SIREN_WARNING, "request queue overflow!");
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            } else {
                siren_printf(SIREN_ERROR, "errorcoid %d in request queue", status);
                continue;
            }
        }

        if (req == nullptr) {
            siren_printf (SIREN_ERROR, "request queue read empty request");
            continue;
        }

        siren_printf(SIREN_INFO, "proxy request thread read msg %d", req->message.msg);
        if (req->message.msg == SIREN_REQUEST_MSG_DESTROY_ON_INIT) {
            req->release();
            return;
        }

        requestWriter.writeMessage(&req->message, req->data);
        siren_printf(SIREN_INFO, "proxy request thread write msg %d to siren", req->message.msg);
        req->release();
    }

    Request req;
    req.message.msg = SIREN_REQUEST_MSG_DESTROY;
    req.message.len = 0;

    siren_printf(SIREN_INFO, "proxy request thread ready to exit, send destroy message to siren");
    requestWriter.writeMessage(&req.message, nullptr);
}

void SirenProxy::stopRequestThread(bool onInit) {
    Request *req = new Request;
    if (onInit) {
        req->message.msg = SIREN_REQUEST_MSG_DESTROY_ON_INIT;
    } else {
        req->message.msg = SIREN_REQUEST_MSG_DESTROY;
    }
    req->message.len = 0;
    req->data = nullptr;
    requestQueue.push(req);

    siren_printf(SIREN_INFO, "waiting proxy request thread exit");
    if (requestThread.joinable()) {
        requestThread.join();
    }
}

void SirenProxy::launchRequestThread() {
    std::thread t(&SirenProxy::requestThreadHandler, this);
    requestThread = std::move(t);
}

void SirenProxy::responseThreadHandler() {
    SirenSocketReader responseReader(&responseChannel);
    responseReader.prepareOnReadSideProcess();
    while (1) {
        Message *msg = nullptr;
        char *data = nullptr;
        int status = SIREN_CHANNEL_OK;
        bool destroy = false;

        {
            std::unique_lock<decltype(launchMutex)> l_(launchMutex);
            requestResponseLaunch = true;
            launchCond.notify_one();
        }

        if ((status = responseReader.pollMessage(&msg, &data)) != SIREN_CHANNEL_OK) {
            siren_printf(SIREN_ERROR, "proxy response thread poll message failed with %d, response thread exit", status);
            break;
        }

        if (msg == nullptr) {
            siren_printf(SIREN_ERROR, "proxy response read null msg");
            continue;
        }

        switch (msg->msg) {
        case SIREN_RESPONSE_MSG_ON_INIT_OK: {
            {
                std::unique_lock<decltype(initMutex)> l_(initMutex);
                waitingInit = true;
                initCond.notify_one();
                return;
            }
        }
        break;
        case SIREN_RESPONSE_MSG_ON_INIT_FAILED: {
            {
                std::unique_lock<decltype(initMutex)> l_(initMutex);
                waitingInit = true;
                sirenBaseInitFailed = true;
                initCond.notify_one();
                siren_printf(SIREN_INFO, "proxy response thread exit since init siren failed");
                return;
            }
        }
        break;
        case SIREN_RESPONSE_MSG_ON_STATE_CHANGED: {
        } break;
        case SIREN_RESPONSE_MSG_ON_VOICE_EVENT: {
        } break;
        case SIREN_RESPONSE_MSG_ON_RAW_VOICE: {
        } break;
        case SIREN_RESPONSE_MSG_ON_CALLBACK: {
        } break;
        case SIREN_RESPONSE_MSG_ON_DESTROY: {
            siren_printf(SIREN_INFO, "proxy response read destroy msg");
            destroy = true;
        }
        break;
        default: {
        }
        }

        if (destroy) {
            break;
        }
    }
}

void SirenProxy::waitingRequestResponseThread() {
    std::unique_lock<decltype(launchMutex)> l_(launchMutex);
    launchCond.wait(l_, [this] {
        return requestResponseLaunch;
    });
}

void SirenProxy::launchResponseThread() {
    std::thread t(&SirenProxy::responseThreadHandler, this);
    responseThread = std::move(t);
}

void SirenProxy::start_siren_process_stream(siren_proc_callback_t *callback) {


}

void SirenProxy::start_siren_raw_stream(siren_raw_stream_callback_t *callback) {

}


void SirenProxy::stop_siren_process_stream() {

}

void SirenProxy::stop_siren_raw_stream() {

}

void SirenProxy::stop_siren_stream() {

}

void SirenProxy::set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) {

}

void SirenProxy::set_siren_steer(float ho, float var) {

}

void SirenProxy::destroy_siren() {
    //stop recording thread
    recordingThread->stop();
    siren_printf(SIREN_INFO, "recording thread stops");
    delete recordingThread;

    //let request exit
    stopRequestThread();

    //response thread will exit epoll and then exit
    if (responseThread.joinable()) {
        responseThread.join();
    }
    siren_printf(SIREN_INFO, "now we can exit...");
}

siren_status_t SirenProxy::rebuild_vt_word_list(const char **vt_word_list, int num) {
    return SIREN_STATUS_OK;
}




}
