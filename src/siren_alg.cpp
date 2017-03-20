#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "siren.h"
#include "siren_config.h"
#include "siren_alg.h"
#include "sutils.h"
#include "isiren.h"

namespace BlackSiren {

PreprocessVoicePackage* PreprocessVoicePackage::allocatePreprocessVoicePackage(int msg, int aec, int size) {
    PreprocessVoicePackage *vp = nullptr;
    char *temp = nullptr;
    int len = 0;
    if (size == 0) {
        len = sizeof(PreprocessVoicePackage);
    } else {
        len = sizeof(PreprocessVoicePackage) + size;
    }

    temp = new char[len];
    if (temp == nullptr) {
        return nullptr;
    }
    memset(temp, 0, len);

    vp = (PreprocessVoicePackage *)temp;
    if (size == 0) {
        vp->data = nullptr;
    } else {
        vp->data = temp + sizeof(PreprocessVoicePackage);
    }

    vp->msg = msg;
    vp->aec = aec;
    vp->size = size;

    return vp;
}

ProcessedVoiceResult* ProcessedVoiceResult::allocateProcessedVoiceResult(int size, int debug, int prop, 
        int hasSL, int hasV, double sl, double energy, double threshold) {
    ProcessedVoiceResult *pvr = nullptr;
    char *temp = nullptr;
    int len = 0;
    if (size == 0) {
        len = sizeof(ProcessedVoiceResult);
    } else {
        len = sizeof(ProcessedVoiceResult) + size;
    }

    temp = new char[len];
    if (temp == nullptr) {
        return nullptr;
    }
    memset(temp, 0, len);

    pvr = (ProcessedVoiceResult *)temp;
    if (size == 0) {
        pvr->data = nullptr;
    } else {
        pvr->data = temp + sizeof(ProcessedVoiceResult);
    }
    pvr->debug = debug;
    pvr->prop = prop;
    pvr->hasSL = hasSL;
    pvr->size = size;
    pvr->hasVoice = hasV;
    pvr->sl = sl;
    pvr->energy = energy;
    pvr->threshold = threshold;

    return pvr;
}

siren_status_t SirenAudioPreProcessor::init() {
    //TODO use config file
    currentLan = config.alg_config.alg_lan;
    int status = 0;

    status = r2ad1_sysinit(config.alg_config.alg_legacy_dir.c_str());
    if (status != 0) {
        siren_printf(SIREN_ERROR, "init ad1 failed");
        ad1Init = false;
        return SIREN_STATUS_ERROR;
    } else {
        ad1Init = true;
    }

    ad1 = r2ad1_create();
    assert (ad1 != nullptr);
    return SIREN_STATUS_OK;
}


void SirenAudioPreProcessor::preprocess(char *rawBuffer, PreprocessVoicePackage **voicePackage) {
    if (!ad1Init) {
        siren_printf(SIREN_ERROR, "ad1 not init yet");
        return;
    }

    int aec = 0;
    if (rawBuffer == nullptr) {
        siren_printf(SIREN_ERROR, "raw buffer is null");
        return;
    }

    aec = r2ad1_putdata(ad1, rawBuffer, frameSize);
    int len = r2ad1_getdatalen(ad1);
    if (len <= 0) {
        siren_printf(SIREN_ERROR, "r2ad1_getdatalen return 0");
        return;
    }

    PreprocessVoicePackage *vp = PreprocessVoicePackage::allocatePreprocessVoicePackage(SIREN_REQUEST_MSG_DATA_PROCESS, aec, len);
    if (vp == nullptr) {
        siren_printf(SIREN_ERROR, "allocatePreprocessVoicePackage FAILED");
        return;
    }

    r2ad1_getdata(ad1, vp->data, len);
    *voicePackage = vp;
}

siren_status_t SirenAudioPreProcessor::destroy() {
    if (ad1Init) {
        r2ad1_sysexit();
    }
    return SIREN_STATUS_OK;
}


siren_status_t SirenAudioVBVProcessor::init() {
    //TODO use config file
    currentLan = config.alg_config.alg_lan;
    int status = 0;

    status = r2ad2_sysinit(config.alg_config.alg_legacy_dir.c_str());
    if (status != 0) {
        siren_printf(SIREN_ERROR, "init ad2 failed");
        ad2Init = false;
        return SIREN_STATUS_ERROR;
    } else {
        ad2Init = true;
    }

    ad2 = r2ad2_create();
    assert (ad2 != nullptr);

    return SIREN_STATUS_OK;
}

siren_status_t SirenAudioVBVProcessor::destroy() {
    if (ad2Init) {
        r2ad2_sysexit();
    }

    return SIREN_STATUS_OK;
}

bool SirenAudioVBVProcessor::hasSlInfo(int prop) {
    if (prop == r2ad_vad_start ||
            prop == r2ad_awake_pre ||
            prop == r2ad_awake_nocmd ||
            prop == r2ad_awake_cmd ||
            prop == r2ad_awake_cancel) {
        return true;
    } else {
        return false;
    }
}

bool SirenAudioVBVProcessor::hasVoice(int prop) {
    if (prop == r2ad_vad_data ||
            prop == r2ad_awake_vad_data ||
            prop == r2ad_debug_audio) {
        return true;
    } else {
        return false;
    }
}

void SirenAudioVBVProcessor::setSysState(int state) {
    r2v_state = (r2v_sys_state)state;
}

void SirenAudioVBVProcessor::setSysSteer(float ho, float ver) {
    r2ad2_steer (ad2, ho, ver);
}

// legacy vbv process just for testing
int SirenAudioVBVProcessor::process(PreprocessVoicePackage *voicePackage,
        std::vector<ProcessedVoiceResult *> &result) {
    if (voicePackage == nullptr) {
        return 0;
    }

    if (!result.empty()) {
        result.clear();
        siren_printf(SIREN_ERROR, "result vector is not empty before process!");
        return 0;
    }
    
    if (voicePackage->data == nullptr || voicePackage->size == 0) {
        siren_printf(SIREN_ERROR, "pre voice package's data is null or len is 0");
        return 0;
    }

    int asrFlag = (r2v_state == r2ssp_state_awake) ? 1 : 0;
    if (setState) {
        setState = false;
        stateCallback((int)r2v_state);
    }
    r2ad2_putaudiodata2(ad2, voicePackage->data, voicePackage->size, voicePackage->aec, 1, asrFlag, asrFlag);
    r2ad_msg_block **ppR2ad_msg_block = nullptr;
    int len = 0;
    int prop = 0;
    int block_num = 0;
    int hasSL = 0;
    int hasV = 0;
    int debug = 0;
    double energy = 0.0;
    double threshold = 0.0;
    double sl = 0.0;
    ProcessedVoiceResult *pProcessedVoiceResult = nullptr;
    r2ad2_getmsg2(ad2, &ppR2ad_msg_block, &block_num);
    if (block_num == 0) {
        return 0;
    }
    
    for (int i = 0; i < block_num; i++) {
        if (ppR2ad_msg_block[i]->iMsgId == r2ad_awake_nocmd) {
            r2v_state = r2ssp_state_awake;
        }

        if (ppR2ad_msg_block[i]->iMsgId == r2ad_sleep) {
            r2v_state = r2ssp_state_sleep;
        }

        len = ppR2ad_msg_block[i]->iMsgDataLen;
        prop = ppR2ad_msg_block[i]->iMsgId;

        if (len != 0 && hasSlInfo(prop)) {
            //clear length
            len = 0;
            hasSL = 1;
            std::string::size_type sz;
            sl = std::stod(std::string(ppR2ad_msg_block[i]->pMsgData), &sz);
        } else {
            hasSL = 0;
            sl = 0.0;
        }

        if (prop == r2ad_debug_audio) {
            debug = 1;
        } else {
            debug = 0;
        }

        if (len != 0 && hasVoice(prop)) {
            hasV = 1;
            energy = static_cast<double>(r2ad2_getenergy_Lastframe(ad2));
            threshold = static_cast<double>(r2ad2_getenergy_Threshold(ad2));
            pProcessedVoiceResult = ProcessedVoiceResult::allocateProcessedVoiceResult(len, debug, prop,
                                    hasSL, hasV, sl, energy, threshold);
            memcpy(pProcessedVoiceResult->data, ppR2ad_msg_block[i]->pMsgData, len);
        } else {
            hasV = 0;
            energy = 0.0;
            threshold = 0.0;
            pProcessedVoiceResult = ProcessedVoiceResult::allocateProcessedVoiceResult(0, debug, prop,
                                    hasSL, hasV, sl, energy, threshold);
        }

        result.push_back(pProcessedVoiceResult);
    }

    return block_num;
}

}
