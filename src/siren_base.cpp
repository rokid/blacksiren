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
    recordingQueue(256, nullptr){

    int channels = config.mic_channel_num;
    int sample = config.mic_sample_rate;
    int byte = config.mic_audio_byte;
    int frameLenInMs = config.mic_frame_length;

    frameSize = channels * sample * byte * 1000 / frameLenInMs;
    frameBuffer = new char[frameSize];

    int rmem = config.siren_recording_socket_rmem;
    setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &rmem, sizeof (rmem));
}

siren_status_t SirenBase::init_siren(const char *path, siren_input_if_t *input) {
    main();
    return SIREN_STATUS_OK;
}

void SirenBase::start_siren_process_stream(siren_proc_callback_t *callback) {

}

void SirenBase::start_siren_raw_stream(siren_raw_stream_callback_t *callback) {

}

void SirenBase::stop_siren_process_stream() {

}

void SirenBase::stop_siren_raw_stream() {

}

void SirenBase::stop_siren_stream() {

}

void SirenBase::set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) {

}

void SirenBase::set_siren_steer(float ho, float var) {

}

void SirenBase::destroy_siren() {

}

siren_status_t SirenBase::rebuild_vt_word_list(const char **vt_word_list, int num) {
    return SIREN_STATUS_OK;
}

void SirenBase::responseThreadHandler() {
    bool first = true;
    while (1) {
        Message *message = nullptr;
        char *data = nullptr;
        int status = SIREN_CHANNEL_OK;
        if (first) {
            first = false;
            Message msg;
            msg.msg = SIREN_RESPONSE_MSG_ON_INIT_OK;
            msg.len = 0;
            resultWriter.writeMessage(&msg, nullptr);
        }

        if ((status = requestReader.pollMessage(&message, &data)) != SIREN_CHANNEL_OK) {
            siren_printf(SIREN_ERROR, "[SirenBase::responseThread] poll message with %d", status);
            continue;
        }

        if (message == nullptr) {
            siren_printf(SIREN_ERROR, "[SirenBase::responseThread] read null message");
            continue;
        }

        switch (message->msg) {
        case SIREN_REQUEST_MSG_START_PROCESS_STREAM: {
            siren_printf(SIREN_INFO, "read message START_PROCESS_STREAM");
        }
        break;
        case SIREN_REQUEST_MSG_STOP_PROCESS_STREAM: {
            siren_printf(SIREN_INFO, "read message STOP_PROCESS_STREAM");
        }
        break;
        case SIREN_REQUEST_MSG_SET_STATE: {
            siren_printf(SIREN_INFO, "read message SET_STATE");
        }
        break;
        case SIREN_REQUEST_MSG_SET_STEER: {
            siren_printf(SIREN_INFO, "read message SET_STEER");
        }
        break;
        case SIREN_REQUEST_MSG_REBUILD_VT_WORD_LIST: {
            siren_printf(SIREN_INFO, "read message REBUILD_VT_WORD_LIST");
        }
        break;
        case SIREN_REQUEST_MSG_DESTROY: {
            siren_printf(SIREN_INFO, "read message DESTROY");
        }
        break;
        default: {
            siren_printf(SIREN_ERROR, "[SirenBase::responseThread] read unknown message");
        }
        }
    }
}

void SirenBase::launchResponseThread() {
    std::thread t(&SirenBase::responseThreadHandler, this);
    responseThread = std::move(t);
}

void SirenBase::responseInitDone() {
    Message msg;
    msg.msg = SIREN_RESPONSE_MSG_ON_INIT_OK;
    msg.len = 0;
    resultWriter.writeMessage(&msg, nullptr);
}

void SirenBase::launchProcessThread() {
    std::thread t(&SirenBase::processThreadHandler, this);
    processThread = std::move(t);
}

void SirenBase::processThreadHandler() {
    siren_printf(SIREN_INFO, "process start");
    SirenAudioProcessor audioProcessor(frameSize);
    if (audioProcessor.init(config) != SIREN_STATUS_OK) {
        siren_printf(SIREN_ERROR, "siren init failed");
        processInitFailed = true;
        return;
    }
    
    while (1) {
        PreprocessVociePackage *pVoicePackage = nullptr;
        ProcessedVoiceResult *pVoiceResult = nullptr;
        int status = 0;
        status = processQueue.pop((void **)&pVoicePackage, nullptr);
        if (status < 0) {
            if (status == -2) {
                siren_printf(SIREN_WARNING, "process queue overflow");
                std::this_thread::sleep_for(std::chrono::microseconds(100));
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
    
            Message msg;
            msg.msg = SIREN_RESPONSE_MSG_ON_VOICE_EVENT;
            msg.len = sizeof(struct ProcessedVoiceResult) + pVoiceResult->size;    
            resultWriter.writeMessage(&msg, pVoiceResult->data);
            pVoicePackage->release();
            pVoiceResult->release();
            continue;
        }
        
        switch (pVoicePackage->msg) {
        case SIREN_REQUEST_MSG_SET_STATE: {      
        } break;
        case SIREN_REQUEST_MSG_SET_STEER: {
        } break;
        case SIREN_REQUEST_MSG_REBUILD_VT_WORD_LIST: {
        } break;
        case SIREN_REQUEST_MSG_DESTROY: {
            pVoicePackage->release();
            audioProcessor.destroy();
            siren_printf(SIREN_INFO, "process thread exit");
        } break;
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
    while (1) {
        {
            std::unique_lock<decltype(recordingMutex)> l_(recordingMutex);
            recordingCond.wait(l_, [this] {
                return recordingStart;
            });
        }

        int status = 0;
        status = read(socket, frameBuffer, frameSize);
        if (status <= 0) {
            siren_printf(SIREN_INFO, "read returns %d", status);
            if (status == -1) {
                break;
            }
        }

        //now we can precess first frame of data
    }

    siren_printf(SIREN_INFO, "siren recording exits now");
}


void SirenBase::main() {
    //launch response thread
    launchProcessThread();
    waitingProcessInit();
    if (processInitFailed) {
        //tell siren proxy we init failed
        Message msg;
        msg.msg = SIREN_RESPONSE_MSG_ON_INIT_FAILED;
        msg.len = 0;
        resultWriter.writeMessage(&msg, nullptr);
    }

    launchResponseThread();
    loopRecording();
    responseThread.join();
}

}
