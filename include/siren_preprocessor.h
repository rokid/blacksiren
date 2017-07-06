#ifndef SIREN_PREPROCESSOR_H_
#define SIREN_PREPROCESSOR_H_

#include "siren_config.h"
#include "common.h"
#include "siren_alg_legacy_helper.h"
#include "sutils.h"

#include <fstream>
#include <vector>
#include "legacy/r2math.h"
#include "legacy/r2mem_i.h"
#include "legacy/r2mem_rdc.h"
#include "legacy/r2mem_aec.h"
#include "legacy/r2mem_o.h"
#include "legacy/r2mem_rs2.h"
#include "legacy/r2mem_buff.h"

namespace BlackSiren {
class SirenPreprocessorImpl {
public:
    SirenPreprocessorImpl(SirenConfig &config_) : config(config_) {
        rsRecordingPath = new std::string[config.alg_config.alg_rs_mics.size()];
        rsRecordingStream = new std::ofstream[config.alg_config.alg_rs_mics.size()];
        for (int i = 0; i < (int)config.alg_config.alg_rs_mics.size(); i++) {
            std::string rspath(config.debug_config.recording_path);
            char buffer[32];
            memset (buffer, 0, sizeof(buffer));
            sprintf(buffer, "/rs_debug%d.pcm", config.alg_config.alg_rs_mics[i]);
            rspath.append(buffer);
            rsRecordingPath[i].assign(rspath);
            siren_printf(SIREN_INFO, "use rs debug path %s", rsRecordingPath[i].c_str());
        }

        aecRecordingPath = new std::string[config.alg_config.alg_aec_mics.size()];
        aecRecordingStream = new std::ofstream[config.alg_config.alg_aec_mics.size()];
        for (int i = 0; i < (int)config.alg_config.alg_aec_mics.size(); i++) {
            std::string aecpath(config.debug_config.recording_path);
            char buffer[32];
            memset (buffer, 0, sizeof(buffer));
            sprintf(buffer, "/aec_debug%d.pcm", config.alg_config.alg_aec_mics[i]);
            aecpath.append(buffer);
            aecRecordingPath[i].assign(aecpath);
            siren_printf(SIREN_INFO, "use aec debug path %s", aecRecordingPath[i].c_str());
        }
    }

    ~SirenPreprocessorImpl() {
        delete []rsRecordingPath;
        delete []rsRecordingStream;
        delete []aecRecordingPath;
        delete []aecRecordingStream;
    }
    SirenPreprocessorImpl(const SirenPreprocessorImpl &) = delete;
    SirenPreprocessorImpl& operator=(const SirenPreprocessorImpl &) = delete;

    int init();
    void destroy();

    int processData(char *pDataIn, int lenIn);
    int getResultLen();
    void getResult(char *pDataOut, int lenOut);
private:
    int processData(char *pDataIn, int lenIn, char *& pData_out, int &lenOut);
    SirenConfig &config;
    PreprocessorUnitAdapter unit;
    PreprocessorMicInfoAdapter micinfo;

    bool rsRecord;
    bool aecRecord;

    std::string *rsRecordingPath;
    std::ofstream *rsRecordingStream;
    std::string *aecRecordingPath;
    std::ofstream *aecRecordingStream;

    bool firstFrm = true;
    bool doAEC = true;
};

}


#endif
