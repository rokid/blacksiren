#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
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

    return true;
}

void RecordingThread::start() {
    siren_printf(SIREN_INFO, "start recording thread");
    pSiren->input_callback->start_input();

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
        pSiren->input_callback->stop_input();
        recordingStart = false;    
    }
}

void RecordingThread::stop() {
    siren_printf(SIREN_INFO, "terminal recording thread");
    std::unique_lock<decltype(termMutex)> lock(termMutex);
    recordingTerm = true;
}

void RecordingThread::recordingFn() {
    while (1) {
        int len = 0;
        {
            std::unique_lock<decltype(termMutex)> lock(termMutex);
            if (recordingTerm) {
                siren_printf(SIREN_INFO, "recording thread exit...");
                break;
            }
        }

        {
            std::unique_lock<decltype(startMutex)> lock(startMutex);
            startCond.wait(lock, [this]{
                    return recordingStart;
            });
            len = pSiren->input_callback->read_input(frameBuffer, frameSize);
        }
      
        if (len != 0) {
            siren_printf(SIREN_ERROR, "read input return %d", len);
            if (currentRetry >= errorRetry) {
                siren_printf(SIREN_INFO, "input error retry max");
                pSiren->input_callback->on_err_input();
            } 
            pSiren->input_callback->stop_input();
            std::this_thread::sleep_for(std::chrono::microseconds(retryTimeout));
            pSiren->input_callback->start_input();
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

siren_status_t SirenProxy::init_siren(const char *path, siren_input_if_t *input) {
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

    requestThread = new ProxyRequestThread(this);
    responseThread = new ProxyResponseThread(this);
    recordingThread = new RecordingThread(this);
    if (!recordingThread->init()) {
        siren_printf(SIREN_ERROR, "init recording thread failed");
        clearThread();
        return SIREN_STATUS_ERROR;
    }
    set_sig_child_handler();

    //fork true siren 
    siren_pid = fork();
    if (siren_pid < 0) {
        siren_printf(SIREN_ERROR, "fork siren failed...");
        clearThread();
        return SIREN_STATUS_ERROR;
    } else if (siren_pid == 0) {
        //gose for siren
     
        unset_sig_child_handler(); 
    } else {
        //gose for parent
        std::thread t(&RecordingThread::recordingFn, recordingThread); 
        recordingThread->setThread(t);
        recordingThread->getThread().detach();

    }
    
    return result;
}


}
