#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include <cstdio>
#include <errno.h>
#include <string.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <iterator>

#include "sutils.h"
#include "siren.h"
#include "isiren.h"
#include "siren_net.h"
#include "siren_base.h"
#include "siren_proxy.h"
#include "siren_config.h"
#include "siren_alg.h"

namespace BlackSiren {

RecordingThread::RecordingThread(SirenProxy *siren) :
    pSiren(siren),
    recordingStart(false),
    recordingTerm(false) {
    SirenConfig config = pSiren->global_config->getConfigFile();
    int channels = config.mic_channel_num;
    int sample = config.mic_sample_rate;
    int byte = config.mic_audio_byte;
    int frameLenInMs = 1000 / config.mic_frame_length;

    frameSize = channels * sample * byte / frameLenInMs;
    siren_printf(SIREN_INFO, "recording thread set frame with %d", frameSize);
    frameBuffer = (char *)malloc(frameSize);

    currentRetry = 0;
    errorRetry = config.siren_input_err_retry_num;
    retryTimeout = config.siren_input_err_retry_timeout;

    if (config.debug_config.mic_array_record) {
        std::string basePath("/mic_array.pcm");
        micRecording .assign(config.debug_config.recording_path).append(basePath);
        siren_printf(SIREN_INFO, "mic_array debug use path %s", micRecording.c_str());
        micRecordingStream.open(micRecording.c_str(), std::ios::out | std::ios::binary);
        if (micRecordingStream.good()) {
            doMicRecording = true;
        } else {
            doMicRecording = false;
            siren_printf(SIREN_ERROR, "mic array recording path not exist");
        }
    }

}

RecordingThread::~RecordingThread() {
    if (frameBuffer != nullptr) {
        free(frameBuffer);
    }

    close(sockets[0]);
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

bool RecordingThread::start() {
    siren_printf(SIREN_INFO, "start recording thread");
    if (0 != pSiren->input_callback->start_input(pSiren->token)) {
        siren_printf(SIREN_INFO, "start recording thread failed");
        return false;
    }

    {
        std::unique_lock<decltype(startMutex)> lock(startMutex);
        recordingStart = true;
        startCond.notify_one();
    }

    return true;
}

void RecordingThread::pause() {
    siren_printf(SIREN_INFO, "stop recording thread");
    {
        std::unique_lock<decltype(startMutex)> lock(startMutex);
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
    bool first = true;
    bool inputStart = false;
    while (1) {
        int len = 0;
        {
            std::unique_lock<decltype(termMutex)> lock(termMutex);
            if (recordingTerm) {
                if (inputStart) {
                    pSiren->input_callback->stop_input(pSiren->token);
                }
                siren_printf(SIREN_INFO, "proxy recording thread exit...");
                break;
            }
        }

        {
            std::unique_lock<decltype(startMutex)> lock(startMutex);
            while (!recordingStart) {
                if (!first) {
                    pSiren->input_callback->stop_input(pSiren->token);
                    inputStart = false;
                }

                startCond.wait(lock);
                if (first) {
                    first = false;
                }


                if (!recordingTerm) {
                    //   pSiren->input_callback->start_input(pSiren->token);
                    inputStart = true;
                }
            }

            if (recordingTerm) {
                siren_printf(SIREN_INFO, "proxy recording thread exit...");
                return;
            }

            len = pSiren->input_callback->read_input(pSiren->token, frameBuffer, frameSize);
            if (doMicRecording) {
                micRecordingStream.write(frameBuffer, frameSize);
            }
            
            //
            if (!recordingStart) {
                continue;
            }
        }

        if (len != 0) {
            siren_printf(SIREN_ERROR, "read input return %d", len);
            if (currentRetry >= errorRetry) {
                siren_printf(SIREN_INFO, "input error retry max");
                pSiren->input_callback->on_err_input(pSiren->token);
                currentRetry = 0;
            } else {
                currentRetry ++ ;
            }
            //pSiren->input_callback->stop_input(pSiren->token);
            //std::this_thread::sleep_for(std::chrono::microseconds(retryTimeout));
            //pSiren->input_callback->start_input(pSiren->token);
            continue;
        } else {
            currentRetry = 0;
        }

        //send to other side
        len = write(sockets[0], frameBuffer, frameSize);
        //siren_printf(SIREN_INFO, "recording write return %d", len);
        if (len < 0) {
            siren_printf(SIREN_ERROR, "write error on socket with %s", strerror(errno));
            //pause();
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
    this->token = token;

    //use share mem
    if (config.siren_use_share_mem) {

        //use socket
    }

    udpAgent.setupConfig(&config);
    if (SIREN_NET_FAILED == udpAgent.prepareSend()) {
        siren_printf(SIREN_ERROR, "prepare send failed");
    }

    //init request channel
    if (!requestChannel.open()) {
        siren_printf(SIREN_ERROR, "request channel open failed");
        delete global_config;
        global_config = nullptr;
        return SIREN_STATUS_ERROR;
    }
    if (!responseChannel.open()) {
        siren_printf(SIREN_ERROR, "response channel open failed");
        delete global_config;
        global_config = nullptr;
        return SIREN_STATUS_ERROR;
    }
    //will release in destroy
    recordingThread = new RecordingThread(this);
    if (!recordingThread->init()) {
        siren_printf(SIREN_ERROR, "init recording thread failed");
        delete global_config;
        global_config = nullptr;
        clearThread();
        return SIREN_STATUS_ERROR;
    }

    waitingInit = false;
    set_sig_child_handler();
    //fork true siren
    siren_pid = fork();
    if (siren_pid < 0) {
        siren_printf(SIREN_ERROR, "fork siren failed...");
        delete global_config;
        delete recordingThread;
        global_config = nullptr;
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
        //load phoneme list
        phonemeGen.loadPhoneme();

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
        input_callback->init_input(token);
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

        Message *req;
        int status = requestQueue.pop((void **)&req, nullptr);
        if (status == -2) {
            siren_printf(SIREN_WARNING, "request queue overflow!");
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }

        if (req == nullptr) {
            siren_printf (SIREN_ERROR, "request queue read empty request");
            continue;
        }

        //siren_printf(SIREN_INFO, "proxy request thread read msg %d", req->msg);
        if (req->msg == SIREN_REQUEST_MSG_DESTROY_ON_INIT) {
            delete [] (char *)req;
            return;
        }

        requestWriter.writeMessage(req);
        //siren_printf(SIREN_INFO, "proxy request thread write msg %d to siren", req->msg);

        delete [] (char *)req;
        if (req->msg == SIREN_REQUEST_MSG_DESTROY) {
            return;
        }
    }

    Message req;
    req.msg = SIREN_REQUEST_MSG_DESTROY;
    req.len = 0;
    req.data = nullptr;

    siren_printf(SIREN_INFO, "proxy request thread ready to exit, send destroy message to siren");
    requestWriter.writeMessage(&req);
}

void SirenProxy::stopRequestThread(bool onInit) {
    int msg;
    if (onInit) {
        msg = SIREN_REQUEST_MSG_DESTROY_ON_INIT;
    } else {
        msg = SIREN_REQUEST_MSG_DESTROY;
    }
    Message *req = allocateMessage(msg, 0);

    requestQueue.push(req);
    siren_printf(SIREN_INFO, "waiting proxy request thread exit");
    if (requestThread.joinable()) {
        requestThread.join();
    }
    siren_printf(SIREN_INFO, "proxy request thread exit..");
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
        int status = SIREN_CHANNEL_OK;
        bool destroy = false;

        {
            std::unique_lock<decltype(launchMutex)> l_(launchMutex);
            requestResponseLaunch = true;
            launchCond.notify_one();
        }

        if ((status = responseReader.pollMessage(&msg)) != SIREN_CHANNEL_OK) {
            siren_printf(SIREN_ERROR, "proxy response thread poll message failed with %d, response thread exit", status);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            //continue;
            return;
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
            }
        }
        break;
        case SIREN_RESPONSE_MSG_ON_INIT_FAILED: {
            {
                std::unique_lock<decltype(initMutex)> l_(initMutex);
                waitingInit = true;
                sirenBaseInitFailed = true;
                initCond.notify_one();

                delete [] (char *)msg;
                return;
            }
        }
        break;
        case SIREN_RESPONSE_MSG_ON_STATE_CHANGED: {
        } break;
        case SIREN_RESPONSE_MSG_ON_VOICE_EVENT: {
            //siren_printf(SIREN_INFO, "on voice event");
            ProcessedVoiceResult *pProcessedVoiceResult = (ProcessedVoiceResult *) msg->data;
            if (pProcessedVoiceResult != nullptr) {
                int size = pProcessedVoiceResult->size;
                char *pData = nullptr;
                //need to fix ptr
                if (size != 0) {
                    pData = (char *)pProcessedVoiceResult + sizeof(ProcessedVoiceResult);
                    pProcessedVoiceResult->data = pData;
                }

                voice_event_t *voice_event = new voice_event_t;
                memset((char *)voice_event, 0, sizeof(voice_event_t));

                voice_event->event = (siren_event_t)pProcessedVoiceResult->prop;
                voice_event->length = pProcessedVoiceResult->size;
                voice_event->background_energy = pProcessedVoiceResult->background_energy;
                voice_event->background_threshold = pProcessedVoiceResult->background_threshold;

                if (pProcessedVoiceResult->hasSL == 1) {
                    voice_event->flag |= SL_MASK;
                    voice_event->sl = pProcessedVoiceResult->sl;
                } else if (pProcessedVoiceResult->hasVoice == 1) {
                    voice_event->flag |= VOICE_MASK;
                    voice_event->buff = pProcessedVoiceResult->data;
                } else if (pProcessedVoiceResult->hasVT == 1) {
                    voice_event->flag |= VT_MASK;
                    voice_event->vt.start = pProcessedVoiceResult->start;
                    voice_event->vt.end = pProcessedVoiceResult->end;
                    voice_event->vt.energy = pProcessedVoiceResult->vt_energy;
                    voice_event->buff = pProcessedVoiceResult->data;
                }
                proc_callback->voice_event_callback(token, voice_event);
                delete voice_event;
            } else {
                siren_printf(SIREN_ERROR, "read voice result nullptr");
            }
        }
        break;
        case SIREN_RESPONSE_MSG_ON_RAW_VOICE: {
        } break;
        case SIREN_RESPONSE_MSG_ON_CALLBACK: {
            int *t = nullptr;
            t = (int *)msg->data;
            int callbackType = t[0];
            switch (callbackType) {
            case SIREN_CALLBACK_ON_STATE_CHANGED: {
                prevState = t[1];
                {
                    std::unique_lock<decltype(stateChangeMutex)> l_(stateChangeMutex);
                    waitStateChange = true;
                    stateChangeCond.notify_one();
                }
                stateChangeCallback(token, prevState);
            }
            break;
            }
        }
        break;
        case SIREN_RESPONSE_MSG_ON_DESTROY: {
            siren_printf(SIREN_INFO, "proxy response read destroy msg");
            destroy = true;
        }
        break;
        default: {
        }
        }

        delete [] (char *)msg;
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

siren_status_t SirenProxy::start_siren_process_stream(siren_proc_callback_t *callback) {
    if (procStreamStart) {
        siren_printf(SIREN_ERROR, "already start...");
        return SIREN_STATUS_ERROR;
    } else {
        proc_callback = callback;
        if (!recordingThread->start()) {
            proc_callback = nullptr;
            siren_printf(SIREN_ERROR, "start failed...");
            return SIREN_STATUS_ERROR;
        }

        procStreamStart = true;
        Message *req = allocateMessage(SIREN_REQUEST_MSG_START_PROCESS_STREAM, 0);
        requestQueue.push((void *)req);
    }
    return SIREN_STATUS_OK;
}
siren_status_t SirenProxy::start_siren_raw_stream(siren_raw_stream_callback_t *callback) {
    return SIREN_STATUS_ERROR;
}


void SirenProxy::stop_siren_process_stream() {
    if (!procStreamStart) {
        siren_printf(SIREN_INFO, "already stop..");
        return;
    } else {
        procStreamStart = false;
    }

    Message *req = allocateMessage(SIREN_REQUEST_MSG_STOP_PROCESS_STREAM, 0);
    requestQueue.push((void *)req);
    recordingThread->pause();
}

void SirenProxy::stop_siren_raw_stream() {

}

void SirenProxy::stop_siren_stream() {

}

void SirenProxy::set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) {
    Message *req = allocateMessage(SIREN_REQUEST_MSG_SET_STATE, sizeof(int));
    int *state_ = (int *)req->data;
    state_[0] = (int)state;
    if (callback != nullptr) {
        stateChangeCallback = callback->state_changed_callback;
        requestQueue.push(req);
    } else {
        waitStateChange = false;
        requestQueue.push(req);
        {
            std::unique_lock<decltype(stateChangeMutex)> l_(stateChangeMutex);
            if (!stateChangeCond.wait_for(l_, std::chrono::seconds(3),
            [this] {
            return waitStateChange;
        })) {
                siren_printf(SIREN_WARNING, "set state change callback timeout");
            }
        }
    }
}

void SirenProxy::set_siren_steer(float ho, float var) {
    Message *req = allocateMessage(SIREN_REQUEST_MSG_SET_STEER, sizeof(float) * 2);
    float *degrees = (float *)req->data;
    degrees[0] = ho;
    degrees[1] = var;
    requestQueue.push(req);
}

int SirenProxy::hasVTWord(const char *word, std::vector<siren_vt_word>::iterator &iterator) {
    if (word == nullptr) {
        return -1;
    }

    iterator = vt_words.begin();
    for (size_t i = 0; iterator != vt_words.end(); ++iterator, i++) {
        if (!strcmp(word, iterator->vt_word.c_str())) {
            return i;
        }
    }

    //don't have
    return 0;
}


siren_vt_t SirenProxy::add_vt_word(siren_vt_word *word, bool use_default_settings) {
    if (word == nullptr) {
        siren_printf(SIREN_ERROR, "word is null!");
        return SIREN_VT_ERROR;
    }

    if (word->vt_type != VT_TYPE_AWAKE &&
            word->vt_type != VT_TYPE_SLEEP &&
            word->vt_type != VT_TYPE_HOTWORD &&
            word->vt_type != VT_TYPE_OTHER) {
        siren_printf(SIREN_ERROR, "vt type illegal");
        return SIREN_VT_ERROR;
    }

    if (word->vt_word.empty()) {
        siren_printf(SIREN_ERROR, "vt word is empty!");
        return SIREN_VT_ERROR;
    }

    if (word->vt_phone.empty()) {
        siren_printf(SIREN_ERROR, "vt phone is empty!");
    }

    std::vector<siren_vt_word>::iterator it;

    int has = hasVTWord(word->vt_word.c_str(), it);
    if (has > 0) {
        siren_printf(SIREN_ERROR, "already has that word!");
        return SIREN_VT_DUP;
    }

    std::string pinyin;
    if (!phonemeGen.pinyin2Phoneme(word->vt_phone.c_str(), pinyin)) {
        siren_printf(SIREN_ERROR, "cannot gen phoneme for %s", word->vt_phone.c_str());
        return SIREN_VT_ERROR;
    }
    siren_printf(SIREN_INFO, "use phoneme %s", pinyin.c_str());
    word->vt_phone = pinyin;

    if (use_default_settings) {
        word->alg_config.vt_block_avg_score = 4.2f;
        word->alg_config.vt_block_min_score = 2.7f;
        word->alg_config.vt_left_sil_det = true;
        word->alg_config.vt_right_sil_det = false;
        word->alg_config.vt_remote_check_with_aec = true;
        word->alg_config.vt_remote_check_without_aec = true;
        word->alg_config.vt_local_classify_check = false;
        word->alg_config.vt_classify_shield = -0.3;
        word->alg_config.nnet_path = "";
    }

    vt_words.push_back(*word);
    /* TODO
    Message *req = allocateMessage(SIREN_REQUEST_MSG_SYNC_VT_WORD_LIST, sizeof(float) * 2);
    requestQueue.push(req);
    */

    Message *req = allocateMessageFromVTWord(vt_words);
    if (req == nullptr) {
        siren_printf(SIREN_ERROR, "allocate sync vt word msg failed");
        return SIREN_VT_ERROR;
    }

    requestQueue.push(req);

    return SIREN_VT_OK;
}

siren_vt_t SirenProxy::remove_vt_word(const char *word) {
    if (word == nullptr) {
        siren_printf(SIREN_ERROR, "word is null");
        return SIREN_VT_ERROR;
    }

    std::vector<siren_vt_word>::iterator it;
    int index =  hasVTWord(word, it);
    if (index == -1) {
        siren_printf(SIREN_ERROR, "no such word: %s", word);
        return SIREN_VT_DUP;
    }

    vt_words.erase(it);
    Message *req = allocateMessageFromVTWord(vt_words);
    if (req == nullptr) {
        siren_printf(SIREN_ERROR, "allocate sync vt word msg failed");
        return SIREN_VT_ERROR;
    }

    requestQueue.push(req);


    return SIREN_VT_OK;
}

int SirenProxy::get_vt_word(siren_vt_word **words) {
    if (words == nullptr) {
        return -1;
    }

    if (stored_words != nullptr) {
        delete []stored_words;
    }

    int size = vt_words.size();
    stored_words = new siren_vt_word[size];
    for (int i = 0; i < size; i++) {
        stored_words[i] = vt_words[i];
    }

    *words = stored_words;
    return size;
}

void SirenProxy::destroy_siren() {
    unset_sig_child_handler();
    //stop recording thread
    recordingThread->stop();
    siren_printf(SIREN_INFO, "recording thread stops");
    delete recordingThread;
    if (global_config) {
        delete global_config;
    }

    //let request exit
    stopRequestThread(false);

    //response thread will exit epoll and then exit
    if (responseThread.joinable()) {
        siren_printf(SIREN_INFO, "waiting proxy response thread exit");
        responseThread.join();
    }

    siren_printf(SIREN_INFO, "waiting siren exit");
    waitpid(siren_pid, nullptr, 0);
    siren_printf(SIREN_INFO, "now we can exit...");
}

void SirenProxy::monitorThreadHandler() {
    while (1) {
        UDPMessage msg;
        memset (&msg, 0, sizeof(UDPMessage));
        if (SIREN_NET_FAILED == udpAgent.pollMessage(msg)) {
            siren_printf(SIREN_ERROR, "poll message failed");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            siren_printf(SIREN_ERROR, "prepare again");
            udpAgent.prepareSend();
            continue;
        }

        if (msg.magic[0] != 'a' ||
                msg.magic[1] != 'a' ||
                msg.magic[2] != 'b' ||
                msg.magic[3] != 'b' ||
                msg.len > 32) {
            siren_printf(SIREN_ERROR, "illegal message");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (net_callback != nullptr) {
            net_callback->net_event_callback(token, msg.data, msg.len);
        }
    }
}

void SirenProxy::launchMonitorThread() {
    std::future<void> udpRecvStartFuture = udpRecvOncePromise.get_future();
    std::thread t(&SirenProxy::monitorThreadHandler, this);
    monitorThread = std::move(t);
    udpRecvStartFuture.wait();
}


void SirenProxy::start_siren_monitor(siren_net_callback_t *callback) {
    this->net_callback = callback;
    if (SIREN_NET_FAILED == udpAgent.prepareRecv()) {
        siren_printf(SIREN_ERROR, "prepare rcv failed");
    }

    //start udp receiver thread
    launchMonitorThread();
    siren_printf(SIREN_INFO, "launch monitor thread done");

}

siren_status_t SirenProxy::broadcast_siren_event(char *data, int len) {
    if (len > 32) {
        siren_printf(SIREN_ERROR, "only support package size less than 32");
        return SIREN_STATUS_ERROR;
    }

    UDPMessage msg;
    memset(&msg, 0, sizeof(UDPMessage));

    msg.magic[0] = 'a';
    msg.magic[1] = 'a';
    msg.magic[2] = 'b';
    msg.magic[3] = 'b';

    msg.len = len;
    memcpy (msg.data, data, len);
    if (SIREN_NET_OK != udpAgent.sendMessage(msg)) {
        siren_printf(SIREN_ERROR, "broadcast failed prepare again");
        udpAgent.prepareSend();
        return SIREN_STATUS_ERROR;
    }
    return SIREN_STATUS_OK;
}


}
