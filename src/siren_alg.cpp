#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>

#include "siren.h"
#include "siren_config.h"
#include "siren_alg.h"
#include "sutils.h"

namespace BlackSiren {

siren_status_t SirenAudioProcessor::init(SirenConfig &config) {
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

    status = r2ad2_sysinit(config.alg_config.alg_legacy_dir.c_str());
    if (status != 0) {
        siren_printf(SIREN_ERROR, "init ad2 failed");
        ad2Init = false;
        return SIREN_STATUS_ERROR;
    } else {
        ad2Init = true;
    }

    status = r2ad3_sysinit(config.alg_config.alg_legacy_dir.c_str());
    if (status != 0) {
        siren_printf(SIREN_ERROR, "init ad3 failed");
        ad3Init = false;
        return SIREN_STATUS_ERROR;
    } else {
        ad3Init = true;
    }

    ad1 = r2ad1_create();
    assert (ad1 != nullptr);

    ad2 = r2ad2_create();
    assert (ad2 != nullptr);

    ad3 = r2ad3_create();
    assert (ad3 != nullptr);

    return SIREN_STATUS_OK;
}

siren_status_t SirenAudioProcessor::destroy() {
    if (ad1Init) {
        r2ad1_sysexit();
    }

    if (ad2Init) {
        r2ad2_sysexit();
    }

    if (ad3Init) {
        r2ad3_sysexit();
    }
    return SIREN_STATUS_OK;
}

void SirenAudioProcessor::process(PreprocessVociePackage *voicePackage, ProcessedVoiceResult **result) {

}

}
