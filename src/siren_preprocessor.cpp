#include "siren_preprocessor.h"

#include <fstream>

#include "sutils.h"
#include "r2ssp.h"

namespace BlackSiren {

static std::ofstream debugStream;
int SirenPreprocessorImpl::init() {
    memset (&unit, 0, sizeof(PreprocessorUnitAdapter));
    memset (&micinfo, 0, sizeof(PreprocessorMicInfoAdapter));
    //init r2ssp
    if (config.debug_config.rs_record) {
        rsRecord = true;
        for (int i = 0; i < (int)config.alg_config.alg_rs_mics.size(); i++) {
            rsRecordingStream[i].open(rsRecordingPath[i].c_str(), std::ios::out|std::ios::binary);
        }
    } else {
        rsRecord = false;
    }

    if (config.debug_config.aec_record) {
        aecRecord = true;
        for (int i = 0; i < (int)config.alg_config.alg_aec_mics.size(); i++) {
            aecRecordingStream[i].open(aecRecordingPath[i].c_str(), std::ios::out|std::ios::binary);
        }
    } else {
        aecRecord = false;
    }

    r2ssp_ssp_init();

    micinfo.m_pMicInfo_in = new r2_mic_info;
    micinfo.m_pMicInfo_in->iMicNum = config.mic_num;
    if (config.mic_num > 0) {
        micinfo.m_pMicInfo_in->pMicIdLst = new int[config.mic_num];
        for (int i = 0; i < config.mic_num; i++) {
            micinfo.m_pMicInfo_in->pMicIdLst[i] = i;
        }
    } else {
        siren_printf(SIREN_ERROR, "wtf mic num is less than 1 ");
        return -1;
    }

    micinfo.m_pMicInfo_rs = new r2_mic_info;
    micinfo.m_pMicInfo_rs->iMicNum = config.alg_config.alg_rs_mics.size();
    if (micinfo.m_pMicInfo_rs->iMicNum > 0) {
        micinfo.m_pMicInfo_rs->pMicIdLst = new int[config.alg_config.alg_rs_mics.size()];
        siren_printf(SIREN_INFO, "rs use:");
        for (int i = 0; i < (int)config.alg_config.alg_rs_mics.size(); i++) {
            micinfo.m_pMicInfo_rs->pMicIdLst[i] = config.alg_config.alg_rs_mics[i];
            siren_printf(SIREN_INFO, "mic%d", config.alg_config.alg_rs_mics[i]);
        }
    } else {
        micinfo.m_pMicInfo_rs->pMicIdLst = nullptr;
        delete micinfo.m_pMicInfo_rs;
        micinfo.m_pMicInfo_rs = nullptr;
        siren_printf(SIREN_INFO, "no rs is need");
    }

    if (config.alg_config.alg_aec) {
        micinfo.m_pMicInfo_aec = new r2_mic_info;
        micinfo.m_pMicInfo_aec->iMicNum = config.alg_config.alg_aec_mics.size();
        if (micinfo.m_pMicInfo_aec->iMicNum > 0) {
            micinfo.m_pMicInfo_aec->pMicIdLst = new int[config.alg_config.alg_aec_mics.size()];
            siren_printf(SIREN_INFO, "aec use:");
            for (int i = 0; i < (int)config.alg_config.alg_aec_mics.size(); i++) {
                micinfo.m_pMicInfo_aec->pMicIdLst[i] = config.alg_config.alg_aec_mics[i];
                siren_printf(SIREN_INFO, "mic%d", config.alg_config.alg_aec_mics[i]);
            }
            doAEC = true;
        } else {
            doAEC = false;
            micinfo.m_pMicInfo_rs->pMicIdLst = nullptr;
            delete micinfo.m_pMicInfo_aec;
            micinfo.m_pMicInfo_aec = nullptr;
            siren_printf(SIREN_INFO, "no aec is need");
        }
    } else {
        doAEC = false;
        micinfo.m_pMicInfo_aec = nullptr;
        siren_printf(SIREN_INFO, "disable aec");
    }

    if (doAEC) {
        micinfo.m_pMicInfo_aec_ref = new r2_mic_info;
        micinfo.m_pMicInfo_aec_ref->iMicNum = config.alg_config.alg_aec_ref_mics.size();
        if (micinfo.m_pMicInfo_aec_ref->iMicNum > 0) {
            micinfo.m_pMicInfo_aec_ref->pMicIdLst = new int[config.alg_config.alg_aec_ref_mics.size()];
            siren_printf(SIREN_INFO, "aec ref use:");
            for (int i = 0; i < (int)config.alg_config.alg_aec_ref_mics.size(); i++) {
                micinfo.m_pMicInfo_aec_ref->pMicIdLst[i] = config.alg_config.alg_aec_ref_mics[i];
                siren_printf(SIREN_INFO, "mic%d", config.alg_config.alg_aec_ref_mics[i]);
            }
        } else {
            doAEC = false;
            delete micinfo.m_pMicInfo_aec_ref;
            micinfo.m_pMicInfo_aec_ref = nullptr;
            siren_printf(SIREN_INFO, "disable aec since no ref mics");
        }
    } else {
        micinfo.m_pMicInfo_aec_ref = nullptr;
    }

    if (doAEC) {
        micinfo.m_pCpuInfo_aec = new r2_mic_info;
        micinfo.m_pCpuInfo_aec->iMicNum = config.alg_config.alg_aec_aff_cpus.size();
        if (micinfo.m_pCpuInfo_aec->iMicNum > 0) {
            micinfo.m_pCpuInfo_aec->pMicIdLst = new int[config.alg_config.alg_aec_aff_cpus.size()];
            siren_printf(SIREN_INFO, "aec cpu uses:");
            for (int i = 0; i < micinfo.m_pCpuInfo_aec->iMicNum; i++) {
                micinfo.m_pCpuInfo_aec->pMicIdLst[i] = config.alg_config.alg_aec_aff_cpus[i];
                siren_printf(SIREN_INFO, "cpu%d", config.alg_config.alg_aec_aff_cpus[i]);
            }
        } else {
            micinfo.m_pCpuInfo_aec->pMicIdLst = nullptr;
        }
    }

    //now init unit

    r2_in_type in_type;
    if (config.mic_audio_byte == 3) {
        in_type = r2_in_int_24;
    } else if (config.mic_num == 10 && config.mic_audio_byte == 4) {
        in_type = r2_in_int_32_10;
    } else if (config.mic_audio_byte == 4) {
        in_type = r2_in_int_32;
    } else {
        siren_printf(SIREN_ERROR, "not support such input");
        return -1;
    }

    unit.m_pMem_in = new r2mem_i(config.mic_num, in_type, micinfo.m_pMicInfo_in);
    unit.m_pMem_rs = new r2mem_rs2(config.mic_num, config.mic_sample_rate, micinfo.m_pMicInfo_rs, false);
    unit.m_pMem_rdc = new r2mem_rdc(micinfo.m_pMicInfo_aec, micinfo.m_pMicInfo_aec_ref, 16000);
    unit.m_pMem_aec = new r2mem_aec(config.mic_num, micinfo.m_pMicInfo_aec, micinfo.m_pMicInfo_aec_ref, micinfo.m_pCpuInfo_aec);
    unit.m_pMem_out = new r2mem_o(config.mic_num, r2_out_float_32, micinfo.m_pMicInfo_aec);
    unit.m_pMem_buff = new r2mem_buff();

    debugStream.open("/data/blacksiren/debug1.pcm", std::ios::out | std::ios::binary);
    return 0;
}

int SirenPreprocessorImpl::processData(char *pDataIn, int lenIn, char *& pData_out, int &lenOut) {
    assert(lenIn == 0 || (lenIn > 0 && pDataIn != NULL));
    
    pData_out = nullptr;
    lenOut = 0;
    float datatmp = 0.0f;

    float** pData_mul = nullptr;
    int inLen_mul = 0;

    unit.m_pMem_in->process(pDataIn, lenIn, pData_mul, inLen_mul);

    unit.m_pMem_rs->process(pData_mul, inLen_mul, pData_mul, inLen_mul);

    unit.m_pMem_rdc->process(pData_mul, inLen_mul);

    if (rsRecord) {
        for (int i = 0; i < (int)config.alg_config.alg_rs_mics.size(); i++) {
            for (int k = 0; k < inLen_mul; k++) {
                datatmp = pData_mul[i][k] / 32768.0f;
                rsRecordingStream[i].write((char *)&datatmp, sizeof(float));
            }
        }
    }
    
    int rt = 0;
    if (doAEC) {
        rt = unit.m_pMem_aec->process(pData_mul, inLen_mul, pData_mul, inLen_mul);
        if (aecRecord) {
            for (int i = 0; i < (int)config.alg_config.alg_aec_mics.size(); i++) {
                for (int k = 0; k < inLen_mul; k++) {
                    datatmp = pData_mul[i][k] / 32768.0f;
                    aecRecordingStream[i].write((char *)&datatmp, sizeof(float));
                }
            }
        }
    }


    if (inLen_mul > 0 && firstFrm) {
        firstFrm = false; 
        for (int i = 0; i < (int)config.alg_config.alg_aec_mics.size(); i++) {
            if (unit.m_pMem_rdc->m_bMicOk[i] == 1) {
                pData_mul[config.alg_config.alg_aec_mics[i]][0] = 1.0f;
            } else {
                siren_printf(SIREN_WARNING, "mic %d detect error", i);
                pData_mul[config.alg_config.alg_aec_mics[i]][0] = 0.0f;
            }
        }
    }

    unit.m_pMem_out->process(pData_mul, inLen_mul, pData_out, lenOut);
    //debugStream.write(pData_out, lenOut);
    return rt;
}

int SirenPreprocessorImpl::processData(char *pDataIn, int lenIn) {
    char *pDataOut = nullptr;
    int lenOut = 0;
    int rt = processData(pDataIn, lenIn, pDataOut, lenOut);
    unit.m_pMem_buff->put(pDataOut, lenOut);
    return rt;
}

int SirenPreprocessorImpl::getResultLen() {
    return unit.m_pMem_buff->getdatalen();
}

void SirenPreprocessorImpl::getResult(char *pDataOut, int lenOut) {
    unit.m_pMem_buff->getdata(pDataOut, lenOut); 
    //debugStream.write(pDataOut, lenOut);
}

void SirenPreprocessorImpl::destroy() {
    if (micinfo.m_pMicInfo_in != nullptr) {
        if (micinfo.m_pMicInfo_in->pMicIdLst != nullptr) {
            delete micinfo.m_pMicInfo_in->pMicIdLst;
            micinfo.m_pMicInfo_in->pMicIdLst = nullptr;
        }
        delete micinfo.m_pMicInfo_in;
        micinfo.m_pMicInfo_in = nullptr;
    }

    if (micinfo.m_pMicInfo_aec_ref != nullptr) {
        if (micinfo.m_pMicInfo_aec_ref->pMicIdLst != nullptr) {
            delete micinfo.m_pMicInfo_aec_ref->pMicIdLst;
            micinfo.m_pMicInfo_aec_ref->pMicIdLst = nullptr;
        }
        delete micinfo.m_pMicInfo_aec_ref;
        micinfo.m_pMicInfo_aec_ref = nullptr;
    }

    if (micinfo.m_pMicInfo_aec != nullptr) {
        if (micinfo.m_pMicInfo_aec->pMicIdLst != nullptr) {
            delete micinfo.m_pMicInfo_aec->pMicIdLst;
            micinfo.m_pMicInfo_aec->pMicIdLst = nullptr;
        }
        delete micinfo.m_pMicInfo_aec;
        micinfo.m_pMicInfo_aec = nullptr;
    }

    if (micinfo.m_pMicInfo_rs != nullptr) {
        if (micinfo.m_pMicInfo_rs->pMicIdLst != nullptr) {
            delete micinfo.m_pMicInfo_rs->pMicIdLst;
            micinfo.m_pMicInfo_rs->pMicIdLst = nullptr;
        }
        delete micinfo.m_pMicInfo_rs;
        micinfo.m_pMicInfo_rs = nullptr;
    }

    if (micinfo.m_pCpuInfo_aec != nullptr) {
        if (micinfo.m_pCpuInfo_aec->pMicIdLst != nullptr) {
            delete micinfo.m_pCpuInfo_aec->pMicIdLst;
            micinfo.m_pCpuInfo_aec->pMicIdLst = nullptr;
        }
        delete micinfo.m_pCpuInfo_aec;
        micinfo.m_pCpuInfo_aec = nullptr;
    }

    delete unit.m_pMem_in;
    delete unit.m_pMem_rs;
    delete unit.m_pMem_aec;
    delete unit.m_pMem_buff;
    delete unit.m_pMem_out;
    delete unit.m_pMem_rdc;

    r2ssp_ssp_exit();
}


}
