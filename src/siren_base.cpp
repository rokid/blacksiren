#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdio>
#include <error.h>
#include <string.h>
#include <sched.h>

#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

#include <fstream>

#include "sutils.h"
#include "siren.h"
#include "isiren.h"
#include "siren_base.h"
#include "siren_config.h"
#include "siren_alg.h"

namespace BlackSiren {
SirenBase::SirenBase(SirenConfig &config_, int socket_, SirenSocketReader &reader_,
                     SirenSocketWriter &writer) :
    processInitFailed(false),
    processThreadInit(false),
    config (config_),
    requestReader(reader_),
    resultWriter(writer),
    socket(socket_),
    recordingExit(false),
    recordingStart(false),
    processQueue(64, nullptr),
    recordingQueue(256, nullptr) {

    int channels = config.mic_channel_num;
    int sample = config.mic_sample_rate;
    int byte = config.mic_audio_byte;
    int frameLenInMs = config.mic_frame_length;

    frameSize = channels * sample * byte / frameLenInMs;
    frameBuffer = new char[frameSize];

    int rmem = config.siren_recording_socket_rmem;
    setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &rmem, sizeof (rmem));
}

SirenBase::~SirenBase() {
    if (frameBuffer != nullptr) {
        delete frameBuffer;
    }
}

siren_status_t SirenBase::init_siren(void *token, const char *path, siren_input_if_t *input) {
    main();
    return SIREN_STATUS_OK;
}

void SirenBase::start_siren_process_stream(siren_proc_callback_t *callback) {
    ((void)callback);
    siren_printf(SIREN_INFO, "base recording thread start");
    std::unique_lock<decltype(recordingMutex)> l_(recordingMutex);
    recordingStart = true;
    recordingCond.notify_one();
}

void SirenBase::start_siren_raw_stream(siren_raw_stream_callback_t *callback) {
    ((void)callback);
}

void SirenBase::stop_siren_process_stream() {
    siren_printf(SIREN_INFO, "base recording thread stop");
    std::unique_lock<decltype(recordingMutex)> l_(recordingMutex);
    recordingStart = false;
}

void SirenBase::stop_siren_raw_stream() {

}

void SirenBase::stop_siren_stream() {

}

void SirenBase::set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) {
    ((void)callback);
    PreprocessVoicePackage *voicePackage =
        PreprocessVoicePackage::allocatePreprocessVoicePackage(SIREN_REQUEST_MSG_SET_STATE,
                0, sizeof(int));
    int *t = nullptr;
    t = (int *)voicePackage->data;
    t[0] = state;
    processQueue.push((void *)voicePackage);
}

void SirenBase::set_siren_steer(float ho, float var) {
    PreprocessVoicePackage *voicePackage =
        PreprocessVoicePackage::allocatePreprocessVoicePackage(SIREN_REQUEST_MSG_SET_STEER,
                0, sizeof(float) * 2);
    float *t = nullptr;
    t = (float *)voicePackage->data;
    t[0] = ho;
    t[1] = var;
    processQueue.push((void *)voicePackage);
}

void SirenBase::destroy_siren() {
    //tell process exit
    PreprocessVoicePackage *voicePackage = new PreprocessVoicePackage;
    voicePackage->msg = SIREN_REQUEST_MSG_DESTROY;
    voicePackage->data = nullptr;
    voicePackage->size = 0;
    processQueue.push((void *)voicePackage);

    //tell proxy response thread exit
    Message msg(SIREN_RESPONSE_MSG_ON_DESTROY);
    resultWriter.writeMessage(&msg);

    recordingExit.store(true, std::memory_order_release);
    if (!recordingStart) {
        std::unique_lock<decltype(recordingMutex)> l_(recordingMutex);
        recordingStart = true;
        recordingCond.notify_one();
    }
}

siren_status_t SirenBase::rebuild_vt_word_list(const char **vt_word_list, int num) {
    return SIREN_STATUS_OK;
}

void SirenBase::responseThreadHandler() {
    int retry = 0;
    while (1) {
        Message *message = nullptr;
        int status = SIREN_CHANNEL_OK;

        if ((status = requestReader.pollMessage(&message)) != SIREN_CHANNEL_OK) {
            siren_printf(SIREN_ERROR, "[SirenBase::responseThread] poll message with %d", status);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            retry++;
            if (retry >= 5) {
                return;
            }
            continue;
        }
        retry = 0;

        if (message == nullptr) {
            siren_printf(SIREN_ERROR, "[SirenBase::responseThread] read null message");
            continue;
        }

        switch (message->msg) {
        case SIREN_REQUEST_MSG_START_PROCESS_STREAM: {
            siren_printf(SIREN_INFO, "read message START_PROCESS_STREAM");
            start_siren_process_stream(nullptr);
        }
        break;
        case SIREN_REQUEST_MSG_STOP_PROCESS_STREAM: {
            siren_printf(SIREN_INFO, "read message STOP_PROCESS_STREAM");
            stop_siren_process_stream();
        }
        break;
        case SIREN_REQUEST_MSG_SET_STATE: {
            siren_printf(SIREN_INFO, "read message SET_STATE");
            if (message->len == sizeof(int)) {
                int *state = (int *)message->data;
                set_siren_state(state[0], nullptr);
            } else {
                siren_printf(SIREN_ERROR, "read SET_STATE but size is not correct, expect %d but %d",
                        (int)sizeof(int), message->len);
            }
        }
        break;
        case SIREN_REQUEST_MSG_SET_STEER: {
            siren_printf(SIREN_INFO, "read message SET_STEER");
            if (message->len == sizeof(float) * 2) {
                float *degrees = (float *)message->data;
                set_siren_steer(degrees[0], degrees[1]);
            } else {
                siren_printf(SIREN_ERROR, "read SET_STEER but size is not correct, expect %d but %d",
                        (int)sizeof(int), message->len);
            }
        }
        break;
        case SIREN_REQUEST_MSG_REBUILD_VT_WORD_LIST: {
            siren_printf(SIREN_INFO, "read message REBUILD_VT_WORD_LIST");
        }
        break;
        case SIREN_REQUEST_MSG_DESTROY: {
            siren_printf(SIREN_INFO, "read message DESTROY all");
            destroy_siren();
            message->release();
            return;
        }
        break;
        default: {
            siren_printf(SIREN_ERROR, "[SirenBase::responseThread] read unknown message");
        }
        }
        message->release();
    }
}

void SirenBase::launchResponseThread() {
    std::thread t(&SirenBase::responseThreadHandler, this);
    responseThread = std::move(t);
}

void SirenBase::responseInitDone() {
    Message msg(SIREN_RESPONSE_MSG_ON_INIT_OK);
    resultWriter.writeMessage(&msg);
}

void SirenBase::launchProcessThread() {
    std::thread t(&SirenBase::processThreadHandler, this);
    processThread = std::move(t);
}

void SirenBase::processThreadHandler() {
    siren_printf(SIREN_INFO, "process start");
    struct sched_param param;
    int maxpri;

    maxpri = sched_get_priority_max(SCHED_FIFO);
    if (maxpri != -1) {
        param.sched_priority = maxpri;
        if (sched_setscheduler(getpid(), SCHED_FIFO, &param) == -1) {
            siren_printf(SIREN_WARNING, "set priority to SCHED_FIFO %d failed", maxpri);
            setpriority(PRIO_PROCESS, getpid(), -20);
        } else {
            siren_printf(SIREN_INFO, "set priority to SCHED_FIFO %d", maxpri);
        }
    } else {
        siren_printf(SIREN_WARNING, "get max priority for SCHED_FIFO failed");
        setpriority(PRIO_PROCESS, getpid(), -20);
    }

    onStateChanged = [this](int state) {
        int *t = nullptr;
        Message *msg = Message::allocateMessage(SIREN_RESPONSE_MSG_ON_CALLBACK, sizeof(int) * 2);
        t = (int *)msg->data;
        t[0] = SIREN_CALLBACK_ON_STATE_CHANGED;
        t[1] = state;
        resultWriter.writeMessage(msg);
        msg->release();
    };

    SirenAudioVBVProcessor audioProcessor(config, onStateChanged);
    if (audioProcessor.init() != SIREN_STATUS_OK) {
        siren_printf(SIREN_ERROR, "siren processor init failed");
        processInitFailed = true;
        processThreadInit = false;
        initCond.notify_one();
        return;
    }

    processThreadInit = true;
    initCond.notify_one();

    while (1) {
        PreprocessVoicePackage *pVoicePackage = nullptr;
        std::vector<ProcessedVoiceResult*> *pVoiceResult = nullptr;
        int status = 0;
        status = processQueue.pop((void **)&pVoicePackage, nullptr);
        if (status < 0) {
            if (status == -2) {
                siren_printf(SIREN_WARNING, "process queue overflow");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else {
                siren_printf(SIREN_ERROR, "process queue pop error");
                continue;
            }
        }

        if (pVoicePackage == nullptr) {
            siren_printf(SIREN_ERROR, "process queue pop null item");
            continue;
        }

        //handle voice process
        if (pVoicePackage->msg == SIREN_REQUEST_MSG_DATA_PROCESS) {
            audioProcessor.process(pVoicePackage, &pVoiceResult);
            if (pVoiceResult == nullptr) {
                siren_printf(SIREN_ERROR, "audio process with null result");
                pVoicePackage->release();
                continue;
            }

            for (ProcessedVoiceResult *p : *pVoiceResult) {
                Message *msg = Message::allocateMessage(SIREN_RESPONSE_MSG_ON_VOICE_EVENT, sizeof(ProcessedVoiceResult) + p->size);
                memcpy(msg->data, p->data, p->size);
                resultWriter.writeMessage(msg);
                p->release();
                p = nullptr;
            }

            pVoicePackage->release();
            pVoiceResult->clear();
            delete pVoiceResult;
            continue;
        }

        switch (pVoicePackage->msg) {
        case SIREN_REQUEST_MSG_SET_STATE: {
            int *t = (int *)pVoicePackage->data;
            int state = t[0];
            siren_printf(SIREN_INFO, "man set state to %d", state);
            audioProcessor.setSysState(state);
        }
        break;
        case SIREN_REQUEST_MSG_SET_STEER: {
            float *t = (float *)pVoicePackage->data;
            float ho = t[0];
            float ver = t[1];
            siren_printf(SIREN_INFO, "set steer %f %f", ho, ver);
            audioProcessor.setSysSteer(ho, ver);
        }
        break;
        case SIREN_REQUEST_MSG_REBUILD_VT_WORD_LIST: {
        } break;
        case SIREN_REQUEST_MSG_DESTROY: {
            pVoicePackage->release();
            audioProcessor.destroy();
            siren_printf(SIREN_INFO, "process thread exit");
            return;
        }
        }

        pVoicePackage->release();
    }
}

void SirenBase::waitingProcessInit() {
    std::unique_lock<decltype(initMutex)> l_(initMutex);
    initCond.wait(l_, [this] {
        return processThreadInit;
    });
}

void SirenBase::loopRecording() {
    SirenAudioPreProcessor preProcessor(frameSize, config);
    if (preProcessor.init() != SIREN_STATUS_OK) {
        siren_printf(SIREN_ERROR, "siren preprocessor init failed");
        //tell process exit
        PreprocessVoicePackage *voicePackage = new PreprocessVoicePackage;
        voicePackage->msg = SIREN_REQUEST_MSG_DESTROY;
        voicePackage->data = nullptr;
        voicePackage->size = 0;
        processQueue.push((void *)voicePackage);

        Message msg(SIREN_RESPONSE_MSG_ON_INIT_FAILED);
        resultWriter.writeMessage(&msg);
        return;
    }

    Message msg(SIREN_RESPONSE_MSG_ON_INIT_OK);
    resultWriter.writeMessage(&msg);
#define CONFIG_RECORDING_DEBUG 1
#ifdef CONFIG_RECORDING_DEBUG
    std::ofstream recordingDebugStream;
    recordingDebugStream.open("/data/test.pcm", std::ios::out | std::ios::binary);
#endif


    while (1) {
        PreprocessVoicePackage *pPreVoicePackage = nullptr;
        {
            std::unique_lock<decltype(recordingMutex)> l_(recordingMutex);
            recordingCond.wait(l_, [this] {
                return recordingStart;
            });
        }

        if (recordingExit.load(std::memory_order_acquire)) {
            siren_printf(SIREN_INFO, "base recording thread request exit");
            return ;
        }

        int status = 0;
        siren_printf(SIREN_INFO, "base read");
        status = read(socket, frameBuffer, frameSize);
        siren_printf(SIREN_INFO, "read %d byte", status);
        if (status <= 0) {
            siren_printf(SIREN_INFO, "read returns %d", status);
            if (recordingExit.load(std::memory_order_acquire)) {
                siren_printf(SIREN_INFO, "base recording thread request exit");
                preProcessor.destroy();
                return;
            } else {
                siren_printf(SIREN_INFO, "base read from socket return %d", status);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }

        //do preprocess
        preProcessor.preprocess(frameBuffer, &pPreVoicePackage);
        if (pPreVoicePackage == nullptr) {
            siren_printf(SIREN_ERROR, "preprocess failed");
            continue;
        }

        //now we can precess first frame of data
#ifdef CONFIG_RECORDING_DEBUG
        if (recordingDebugStream.is_open()) {
            recordingDebugStream.write(pPreVoicePackage->data, pPreVoicePackage->size);
            pPreVoicePackage->release();
        } else {
            siren_printf(SIREN_WARNING, "recording debug frame is not open!");
        }
#endif

    }

    siren_printf(SIREN_INFO, "siren recording exits now");
}


void SirenBase::main() {
    //launch response thread
    launchProcessThread();
    siren_printf(SIREN_INFO, "waiting process thread");
    waitingProcessInit();
    siren_printf(SIREN_INFO, "process thread started...");
    if (processInitFailed) {
        siren_printf(SIREN_ERROR, "process init failed");
        //tell siren proxy we init failed
        Message msg(SIREN_RESPONSE_MSG_ON_INIT_FAILED);
        resultWriter.writeMessage(&msg);
    }

    //we need to known proxy response thread has started
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //start response thread
    launchResponseThread();
    loopRecording();
    siren_printf(SIREN_INFO, "waiting base response thread confirm exiting");
    if (responseThread.joinable()) {
        responseThread.join();
    }

    siren_printf(SIREN_INFO, "waiting process thread exit");
    if (processThread.joinable()) {
        processThread.join();
    }
}

}
