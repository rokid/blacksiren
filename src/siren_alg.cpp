#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>

#include "siren.h"
#include "siren_config.h"
#include "siren_alg.h"
#include "sutils.h"
#include "isiren.h"

namespace BlackSiren {

PreprocessVoicePackage* allocatePreprocessVoicePackage(int msg, int aec, int size) {
    PreprocessVoicePackage *vp = nullptr;
    char *temp = nullptr;
    if (size == 0) {
        temp = new char[sizeof(PreprocessVoicePackage)];
        memset(temp, 0, sizeof(PreprocessVoicePackage) + size);
        if (temp == nullptr) {
            return nullptr;
        }
        vp = (PreprocessVoicePackage *)temp;
        vp->data = nullptr;
    } else {
        char *temp = new char[sizeof(PreprocessVoicePackage) + size];
        memset(temp, 0, sizeof(PreprocessVoicePackage) + size);
        if (temp == nullptr) {
            return nullptr;
        }
        vp = (PreprocessVoicePackage *)temp;
        vp->data = temp + sizeof(PreprocessVoicePackage);
    }

    vp->msg = msg;
    vp->aec = aec;
    vp->size = size;
    
    return vp;
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
    int aec = 0;
    if (rawBuffer == nullptr) {
        return;
    }

    aec = r2ad1_putdata(ad1, rawBuffer, frameSize);
    int len = r2ad1_getdatalen(ad1);
    if (len <= 0) {
        return;
    }

    PreprocessVoicePackage *vp = allocatePreprocessVoicePackage(SIREN_REQUEST_MSG_DATA_PROCESS, aec, len);
    if (vp == nullptr) {
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

// legacy vbv process just for testing
void SirenAudioVBVProcessor::process(PreprocessVoicePackage *voicePackage, ProcessedVoiceResult **result) {

}

}
