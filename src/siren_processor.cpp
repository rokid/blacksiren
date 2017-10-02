#include "siren_processor.h"
#include "siren_config.h"
#include "sutils.h"
#include "siren_alg_legacy_helper.h"

#include <fstream>
#include <vector>

#include "NNVadIntf.h"
#include "r2ssp.h"
#include "legacy/r2ad2.h"
#include "legacy/zvbvapi.h"
#include "legacy/zvtapi.h"

namespace BlackSiren {

void SirenProcessorImpl::getErrorInfo(float** data_mul, std::vector<int> &errorMic) {
    for (int i = 0; i < micinfo.m_pMicInfo_in->iMicNum; i++) {
        int id = micinfo.m_pMicInfo_in->pMicIdLst[i];
        if (fabs(data_mul[id][0]) < 0.5f) {
            errorMic.push_back(id);
        }
    }
}

bool SirenProcessorImpl::fixErrorMic(std::vector<int> &errorMic) {
    if (errorMic.empty()) {
        return false;
    }

    int cur = 0;
    for (int i = 0; i < micinfo.m_pMicInfo_bf->iMicNum; i++) {
        bool exit = false;
        for (int j = 0; j < (int)errorMic.size(); j++) {
            if (micinfo.m_pMicInfo_bf->pMicIdLst[i] == errorMic[j]) {
                exit = true;
                break;
            }
        }

        if (!exit) {
            if (i != cur) {
                micinfo.m_pMicInfo_bf->pMicIdLst[cur] = micinfo.m_pMicInfo_bf->pMicIdLst[i];
            }
            cur++;
        }
    }

    if (cur == micinfo.m_pMicInfo_bf->iMicNum) {
        return false;
    } else {
        micinfo.m_pMicInfo_bf->iMicNum = cur;
        return true;
    }
}

void SirenProcessorImpl::ProcessState::updateAndDumpStatus(int lenin, int aecflag, int awakeflag, int sleepflag, int asrflag) {
    if (awakeflag != this->awakeFlag ||
            sleepflag != this->sleepFlag ||
            asrflag != this->asrFlag ||
            aecflag != this->aecFlag) {
        this->lastDuration += 0;
        this->aecFlag = aecflag;
        this->awakeFlag = awakeflag;
        this->sleepFlag = sleepflag;
        this->asrFlag = asrflag;

        siren_printf(SIREN_INFO, "siren processor status changed ------ AwakeFlag %s SleepFlag %s AsrFlag %s AecFlag %s",
                     ((awakeflag == 1) ? "true" : "false"),
                     ((sleepflag == 1) ? "true" : "false"),
                     ((asrflag == 1) ? "true" : "false"),
                     ((aecflag == 1) ? "true" : "false"));
    } else {
        this->lastDuration += lenin;
        if (this->lastDuration > this->frmSize * 5000) {
            siren_printf(SIREN_INFO, "siren processor status -----  AwakeFlag %s SleepFlag %s AsrFlag %s AecFlag %s",
                         ((awakeflag == 1) ? "true" : "false"),
                         ((sleepflag == 1) ? "ture" : "false"),
                         ((asrflag == 1) ? "true" : "false"),
                         ((aecflag == 1) ? "true" : "false"));
            this->lastDuration = 0;
        }
    }

}

int SirenProcessorImpl::init() {
    memset(&unit, 0, sizeof(ProcessorUnitAdapter));
    memset(&micinfo, 0, sizeof(ProcessorMicInfoAdapter));

    if (config.debug_config.bf_record) {
        bf_record = true;
        bfRecordingStream.open(bfRecordingPath, std::ios::out | std::ios::binary);
        if (!bfRecordingStream.good()) {
            bf_record = false;
            siren_printf(SIREN_WARNING, "cannot open file %s", bfRecordingPath.c_str());
        }
    } else {
        bf_record = false;
    }

    if (config.debug_config.bf_raw_record) {
        bf_raw_record = true;
        bfRawRecordingStream.open(bfRawRecordingPath, std::ios::out | std::ios::binary);
        if (!bfRawRecordingStream.good()) {
            bf_raw_record = false;
            siren_printf(SIREN_WARNING, "cannot open file %s", bfRawRecordingPath.c_str());
        }
    } else {
        bf_raw_record = false;
    }

    if (config.debug_config.vad_record) {
        vad_record = true;
        vadRecordingStream.open(vadRecordingPath, std::ios::out | std::ios::binary);
        if (!vadRecordingStream.good()) {
            vad_record = false;
            siren_printf(SIREN_WARNING, "cannot open file %s", vadRecordingPath.c_str());
        }

    } else {
        vad_record = false;
    }

    if (config.debug_config.debug_opu_record) {
        opu_record = true;
        opuRecordingStream.open(opuRecordingPath, std::ios::out | std::ios::binary);
        if (!opuRecordingStream.good()) {
            opu_record = false;
            siren_printf(SIREN_WARNING, "cannot open file %s", opuRecordingPath.c_str());
        }
    } else {
        opu_record = false;
    }

    int mic_num = config.mic_num;
    micinfo.mic_pos = new float[mic_num * 3];
    micinfo.mic_i2s_delay = new float[mic_num];
    memset(micinfo.mic_pos, 0, sizeof(float) * mic_num * 3);
    for (int i = 0; i < mic_num; i++) {
        micinfo.mic_i2s_delay[i] = 0.0f;
    }

    for (int i = 0; i < mic_num; i++) {
        for (int j = 0; j < 3; j++) {
            micinfo.mic_pos[i * 3 + j] =
            config.alg_config.alg_mic_pos[i].pos[j];
        }
    }

    for (int i = 0; i < (int)config.alg_config.alg_need_i2s_delay_mics.size(); i++) {
        int mic_idx = config.alg_config.alg_need_i2s_delay_mics[i];
        if (mic_idx >= mic_num) {
            siren_printf(SIREN_ERROR, "wtf idx in need i2s delay larger than mic num");
            continue;
        }
        micinfo.mic_i2s_delay[mic_idx] = config.alg_config.alg_i2s_delay_mics[i];
        siren_printf(SIREN_INFO, "mic %d use delay %f", mic_idx, config.alg_config.alg_i2s_delay_mics[i]);
    }

    int aec_mic_num = config.alg_config.alg_aec_mics.size();
    micinfo.m_pMicInfo_in = new r2_mic_info;
    micinfo.m_pMicInfo_in->iMicNum = aec_mic_num;
    if (aec_mic_num > 0) {
        micinfo.m_pMicInfo_in->pMicIdLst = new int[aec_mic_num];
        for (int i = 0; i < aec_mic_num; i++) {
            micinfo.m_pMicInfo_in->pMicIdLst[i]
                = config.alg_config.alg_aec_mics[i];
        }
    } else {
        siren_printf(SIREN_ERROR, "wtf aec mic num is less than 1");
        return -1;
    }

    int sl_mic_num = config.alg_config.alg_sl_mics.size();
    micinfo.m_pMicInfo_bf = new r2_mic_info;
    micinfo.m_pMicInfo_bf->iMicNum = sl_mic_num;
    if (sl_mic_num > 0) {
        micinfo.m_pMicInfo_bf->pMicIdLst = new int[sl_mic_num];
        for (int i = 0; i < sl_mic_num; i++) {
            micinfo.m_pMicInfo_bf->pMicIdLst[i]
                = config.alg_config.alg_sl_mics[i];
        }
    } else {
        siren_printf(SIREN_ERROR, "wtf sl mic num is less than 1");
        return -1;
    }


    VAD_SysInit();
    siren_printf(SIREN_INFO, "VAD INIT OK!");

    r2ssp_ssp_init();
    siren_printf(SIREN_INFO, "R2SSP INIT OK!");

    //load default vt words
    int defaultVTWordNum = config.alg_config.def_vt_configs.size();
    siren_printf(SIREN_INFO, "load default vt word %d", defaultVTWordNum);
    micinfo.currentWordNum = defaultVTWordNum;
    micinfo.m_pWordLst = new WordInfo[micinfo.currentWordNum];

    for (int i = 0; i < micinfo.currentWordNum; i++) {
        micinfo.m_pWordLst[i].iWordType = (WordType)config.alg_config.def_vt_configs[i].vt_type;
        strcpy(micinfo.m_pWordLst[i].pWordContent_UTF8,
               config.alg_config.def_vt_configs[i].vt_word.c_str());
        strcpy(micinfo.m_pWordLst[i].pWordContent_PHONE,
               config.alg_config.def_vt_configs[i].vt_phone.c_str());
        micinfo.m_pWordLst[i].fBlockAvgScore =
            config.alg_config.def_vt_configs[i].vt_avg_score;
        micinfo.m_pWordLst[i].fBlockMinScore =
            config.alg_config.def_vt_configs[i].vt_min_score;

        micinfo.m_pWordLst[i].bLeftSilDet =
            config.alg_config.def_vt_configs[i].vt_left_sil_det;
        micinfo.m_pWordLst[i].bRightSilDet =
            config.alg_config.def_vt_configs[i].vt_right_sil_det;

        micinfo.m_pWordLst[i].bRemoteAsrCheckWithAec =
            config.alg_config.def_vt_configs[i].vt_remote_check_with_aec;
        micinfo.m_pWordLst[i].bRemoteAsrCheckWithNoAec =
            config.alg_config.def_vt_configs[i].vt_remote_check_without_aec;

        micinfo.m_pWordLst[i].bLocalClassifyCheck =
            config.alg_config.def_vt_configs[i].vt_local_classify_check;
        micinfo.m_pWordLst[i].fClassifyShield =
            config.alg_config.def_vt_configs[i].vt_classify_shield;

        bool nnet_valid = false;
        if (!config.alg_config.def_vt_configs[i].vt_nnet_path.empty()) {
            strcpy(micinfo.m_pWordLst[i].pLocalClassifyNnetPath,
                   config.alg_config.def_vt_configs[i].vt_nnet_path.c_str());
            nnet_valid = true;
        } else {
            nnet_valid = false;
        }

        siren_printf(SIREN_INFO, "load word%d", i);
        siren_printf(SIREN_INFO, "word content: %s", micinfo.m_pWordLst[i].pWordContent_UTF8);
        siren_printf(SIREN_INFO, "phone: %s", micinfo.m_pWordLst[i].pWordContent_PHONE);
        siren_printf(SIREN_INFO, "block avg score %f", micinfo.m_pWordLst[i].fBlockAvgScore);
        siren_printf(SIREN_INFO, "block min score %f", micinfo.m_pWordLst[i].fBlockMinScore);
        siren_printf(SIREN_INFO, "left sil det %s", micinfo.m_pWordLst[i].bLeftSilDet ? "true" : "false");
        siren_printf(SIREN_INFO, "right sil det %s", micinfo.m_pWordLst[i].bRightSilDet ? "true" : "false");
        siren_printf(SIREN_INFO, "remote asr check with aec %s",
                     micinfo.m_pWordLst[i].bRemoteAsrCheckWithAec ? "true" : "false");
        siren_printf(SIREN_INFO, "remote asr check without aec %s",
                     micinfo.m_pWordLst[i].bRemoteAsrCheckWithNoAec ? "true" : "false");
        siren_printf(SIREN_INFO, "local classify check %s", micinfo.m_pWordLst[i].bLocalClassifyCheck ? "true" : "false");
        siren_printf(SIREN_INFO, "classify shield %f", micinfo.m_pWordLst[i].fClassifyShield);

        if (nnet_valid) {
            siren_printf(SIREN_INFO, "nnet valid path %s",
                         micinfo.m_pWordLst[i].pLocalClassifyNnetPath);
        } else {
            siren_printf(SIREN_INFO, "nnet invalid maybe useless");
        }
    }

    //init unit
    unit.m_pMem_in = new r2mem_i(mic_num, r2_in_float_32, micinfo.m_pMicInfo_in);

    unit.m_pMem_vbv3 = new r2mem_vbv3(mic_num, micinfo.mic_pos,
                                      micinfo.mic_i2s_delay, micinfo.m_pMicInfo_bf,
                                      config.alg_config.alg_vt_dnnmod.c_str(),
                                      config.alg_config.alg_vt_phomod.c_str());

    unit.m_pMmem_bf = new r2mem_bf(mic_num, micinfo.mic_pos,
                                   micinfo.mic_i2s_delay, micinfo.m_pMicInfo_bf);

    unit.m_pMem_vad2 = new r2mem_vad2(config.alg_config.alg_vad_baserange,
                                      config.alg_config.alg_vad_dynrange_min,
                                      config.alg_config.alg_vad_dynrange_max);

    //use opus
    if (config.alg_config.alg_opus_compress) {
        unit.m_pMem_cod = new r2mem_cod(r2ad_cod_opu);
    } else {
        unit.m_pMem_cod = new r2mem_cod(r2ad_cod_pcm);
    }

    //set default word
    unit.m_pMem_vbv3->SetWords(micinfo.m_pWordLst, micinfo.currentWordNum);
    memset (&allocator, 0, sizeof(TinyAllocator));

    allocator.msgNumCurr = 0;
    allocator.msgNumTotal = 1000;
    allocator.msgLst = (r2ad_msg_block **)malloc(sizeof(r2ad_msg_block *) * allocator.msgNumTotal);

    allocator.colNoNew = R2_AUDIO_SAMPLE_RATE * 5;
    allocator.dataNoNew = R2_SAFE_NEW_AR2(allocator.dataNoNew, float, config.mic_num, allocator.colNoNew);

    state.frmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS;
    state.lastDuration = 0;
    state.aecFlag = -1;
    state.awakeFlag = -1;
    state.sleepFlag = -1;
    state.asrFlag = -1;
    state.firstFrm = true;

    state.asrMsgCheckFlag = 0;
    return 0;
}


void SirenProcessorImpl::destroy() {
    if (micinfo.mic_pos != nullptr) {
        delete []micinfo.mic_pos;
        micinfo.mic_pos = nullptr;
    }

    if (micinfo.mic_i2s_delay != nullptr) {
        delete []micinfo.mic_i2s_delay;
        micinfo.mic_i2s_delay = nullptr;
    }

    if (micinfo.m_pMicInfo_in != nullptr) {
        delete micinfo.m_pMicInfo_in;
        micinfo.m_pMicInfo_in = nullptr;
    }

    if (micinfo.m_pMicInfo_bf != nullptr) {
        delete micinfo.m_pMicInfo_bf;
        micinfo.m_pMicInfo_bf = nullptr;
    }

    if (micinfo.m_pWordLst != nullptr) {
        delete[] micinfo.m_pWordLst;
        micinfo.m_pWordLst = nullptr;
    }

    micinfo.currentWordNum = 0;

    if (unit.m_pMem_in != nullptr) {
        delete unit.m_pMem_in;
        unit.m_pMem_in = nullptr;
    }

    if (unit.m_pMem_cod != nullptr) {
        delete unit.m_pMem_cod;
        unit.m_pMem_cod = nullptr;
    }

    if (unit.m_pMmem_bf != nullptr) {
        delete unit.m_pMmem_bf;
        unit.m_pMmem_bf = nullptr;
    }

    if (unit.m_pMem_vad2 != nullptr) {
        delete unit.m_pMem_vad2;
        unit.m_pMem_vad2 = nullptr;
    }

    if (unit.m_pMem_vbv3 != nullptr) {
        delete unit.m_pMem_vbv3;
        unit.m_pMem_vbv3 = nullptr;
    }

    r2ssp_ssp_exit();
    VAD_SysExit();


    clearMsgLst();

    free(allocator.msgLst);
    R2_SAFE_DEL_AR2(allocator.dataNoNew);
}


void SirenProcessorImpl::process(char *datain, int lenin, int aecflag, int awakeflag, int sleepflag, int asrflag, int hotwordflag) {

    clearMsgLst();
    bool aec = (aecflag == 1);
    bool asr = (asrflag == 1);
    bool awake = (awakeflag == 1);
    bool sleep = (sleepflag == 1);
    bool hotword = (hotwordflag == 1);

    if (asr) {
        state.asr = true;
//    } else if (!asr && !config.alg_config.alg_vad_enable){
//        state.asr = false;
    }
 
    float datatmp = 0.0f;
    float** data_mul = nullptr;
    float* data_sig = nullptr;
    int len_mul = 0;
    int len_sig = 0;

    unit.m_pMem_in->process(datain, lenin, data_mul, len_mul);

    //fix mic
    if (state.firstFrm && len_mul > 0) {
        state.firstFrm = false;
        std::vector<int> errorMics;
        getErrorInfo(data_mul, errorMics);
        if (fixErrorMic(errorMics)) {
            siren_printf(SIREN_WARNING, "detect mic error:");
            for (int i = 0; i < micinfo.m_pMicInfo_bf->iMicNum; i++) {
                siren_printf(SIREN_WARNING, "error mic %d", micinfo.m_pMicInfo_bf->pMicIdLst[i]);
            }
            delete unit.m_pMem_vbv3;
            delete unit.m_pMmem_bf;

            unit.m_pMem_vbv3 = new r2mem_vbv3(config.mic_num, micinfo.mic_pos,
                                              micinfo.mic_i2s_delay, micinfo.m_pMicInfo_bf,
                                              config.alg_config.alg_vt_dnnmod.c_str(),
                                              config.alg_config.alg_vt_phomod.c_str());

            unit.m_pMmem_bf = new r2mem_bf(config.mic_num, micinfo.mic_pos,
                                           micinfo.mic_i2s_delay, micinfo.m_pMicInfo_bf);
            unit.m_pMem_vbv3->SetWords(micinfo.m_pWordLst, micinfo.currentWordNum);
        }
    }

    state.updateAndDumpStatus(len_mul, aecflag, awakeflag, sleepflag, asrflag);

    //bf
    unit.m_pMmem_bf->process(data_mul, len_mul, data_sig, len_sig);

    if (config.alg_config.alg_bf_scaling == 0.0f) {
        config.alg_config.alg_bf_scaling = 1.0f;
    }

    for (int i = 0; i < len_sig; i++) {
        if (config.alg_config.alg_bf_scaling > 0) {
            data_sig[i] = data_sig[i] * config.alg_config.alg_bf_scaling;
        } else {
            data_sig[i] = data_sig[i] / abs(config.alg_config.alg_bf_scaling);
        }
    }

    if (bf_record) {
        for (int i = 0; i < len_sig; i++) {
            datatmp = data_sig[i] ;
            bfRecordingStream.write((char *)&datatmp, sizeof(float));
        }
    }

    //vbv
    int vbv_result = unit.m_pMem_vbv3->Process(data_mul, len_mul,
                     state.dataOutput, aec, awake, sleep, hotword);

    if ((vbv_result & R2_VT_WORD_CANCEL) != 0) {
        assert((vbv_result & R2_VT_WORD_PRE) != 0);
    }

    bool pre = ((vbv_result & R2_VT_WORD_PRE) != 0 && (vbv_result & R2_VT_WORD_CANCEL) == 0);

    bool awakePre = awake && pre && (unit.m_pMem_vbv3->m_pWordInfo->iWordType == WORD_AWAKE);
    bool awakeNoCmd = awake && ((vbv_result & R2_VT_WORD_DET_NOCMD) != 0 && unit.m_pMem_vbv3->m_pWordInfo->iWordType == WORD_AWAKE);
    bool awakeCmd = awake && ((vbv_result & R2_VT_WORD_DET_CMD) != 0 && unit.m_pMem_vbv3->m_pWordInfo->iWordType == WORD_AWAKE);

    bool sleepNoCmd = sleep && ((vbv_result & R2_VT_WORD_DET_NOCMD) != 0 && unit.m_pMem_vbv3->m_pWordInfo->iWordType == WORD_SLEEP);
    bool sleepCmd = sleep && ((vbv_result & R2_VT_WORD_DET_CMD) != 0 && unit.m_pMem_vbv3->m_pWordInfo->iWordType == WORD_SLEEP);

    bool hotwordNoCmd = hotword && ((vbv_result & R2_VT_WORD_DET_NOCMD) != 0 && unit.m_pMem_vbv3->m_pWordInfo->iWordType == WORD_HOTWORD);
    bool hotwordCmd = hotword && ((vbv_result & R2_VT_WORD_DET_CMD) != 0 && unit.m_pMem_vbv3->m_pWordInfo->iWordType == WORD_HOTWORD);

    bool cmd = awakeCmd || sleepCmd || hotwordCmd;
    bool noCmd = awakeNoCmd || sleepNoCmd || hotwordNoCmd;

    int forceStart = 0;

    //have pre
    if (pre) {
        resetASR();
        memset(slinfo, 0, sizeof(float) * 3);
        //save last bf we will try to focus on pre direction
        memcpy(slinfo, unit.m_pMmem_bf->m_fSlInfo, sizeof(float) * 3);
        unit.m_pMmem_bf->steer(unit.m_pMem_vbv3->m_pWordDetInfo->fWordSlInfo[0],
                               unit.m_pMem_vbv3->m_pWordDetInfo->fWordSlInfo[1]);

        //reset
        int start = unit.m_pMem_vbv3->m_pWordDetInfo->iWordPos_Start;
        int end = unit.m_pMem_vbv3->m_pWordDetInfo->iWordPos_End;

        //go front 200ms
        start += 20 * state.frmSize;

        if (start > allocator.colNoNew) {
            allocator.colNoNew = start * 2;
            R2_SAFE_DEL_AR2(allocator.dataNoNew);
            allocator.dataNoNew = R2_SAFE_NEW_AR2(allocator.dataNoNew, float, config.mic_num, allocator.colNoNew);
        }

        unit.m_pMem_vbv3->GetLastAudio(allocator.dataNoNew,
                                       start, 0);
        if(!sleepNoCmd){
            unit.m_pMmem_bf->process(allocator.dataNoNew, start, data_sig, len_sig);
            data_sig += 15 * state.frmSize;
            len_sig -= 15 * state.frmSize;
            for (int i = 0; i < len_sig; i++) {
                if (config.alg_config.alg_bf_scaling > 0) {
                    data_sig[i] = data_sig[i] * config.alg_config.alg_bf_scaling;
                } else {
                    data_sig[i] = data_sig[i] / abs(config.alg_config.alg_bf_scaling);
                }
            }
        }
        forceStart = 1;
    }

    //siren_printf(SIREN_INFO, "vad2 process");
    int vad2 = unit.m_pMem_vad2->process(data_sig, len_sig, 0,
                                         0, forceStart, data_sig, len_sig);
    if (!pre) {
        if (vad_record) {
            for (int i = 0; i < len_sig; i++) {
                float tmp = data_sig[i] / 32768.0f;
                vadRecordingStream.write((char *)&tmp, sizeof(float));
            }
        }
    }

    if (vad2 & r2vad_audio_begin) {
        siren_printf(SIREN_INFO, "vad audio begin");
        state.asr = asr;
        state.vadStart = true;
        state.dataOutput = false;
        state.canceled = false;
        state.awke = false;
        unit.m_pMem_cod->reset();
        if (awakePre) {
            state.awke = true;
//            if(state.dataOutput && !sleepNoCmd){
//                state.dataOutput = false;
//                addMsg(r2ad_sleep, unit.m_pMmem_bf->getinfo_sl());
//            }
            addMsg(r2ad_awake_pre, unit.m_pMmem_bf->getinfo_sl());
            addMsg(r2ad_debug_audio, "");
            state.lastAwakeInfo.assign(unit.m_pMem_vbv3->m_pWordInfo->pWordContent_UTF8);
        }
    }

    if (noCmd) {
        if (sleepNoCmd) {
            addMsg(r2ad_sleep, unit.m_pMmem_bf->getinfo_sl());
        }

        if (awakeNoCmd) {
            addMsg(r2ad_awake_nocmd, unit.m_pMmem_bf->getinfo_sl());
        }

        if (hotwordNoCmd) {
            addMsg(r2ad_hotword, unit.m_pMmem_bf->getinfo_sl());
        }

        if (config.alg_config.alg_vad_enable) {
            unit.m_pMem_vad2->setvadendparam(-1);
        } else {
            unit.m_pMem_vad2->setvadendparam(2000);
        }
    }

    if (cmd) {
        if (sleepCmd) {

        }

        if (awakeCmd) {
            addMsg(r2ad_awake_cmd, unit.m_pMmem_bf->getinfo_sl());
        }

        if (hotwordCmd) {
        }

        if (config.alg_config.alg_vad_enable) {
            unit.m_pMem_vad2->setvadendparam(-1);
        } else {
            unit.m_pMem_vad2->setvadendparam(2000);
        }
    }

    if (state.vadStart) {
        unit.m_pMem_cod->process(data_sig, len_sig);
        if (!pre) {
            if (opu_record) {
                for (int i = 0; i < len_sig; i++) {
                    float tmp = data_sig[i] / 32768.0f;
                    opuRecordingStream.write((char *)&tmp, sizeof(float));
                }
            }
        }

        if (!state.dataOutput) {
            if (cmd || noCmd) {
                state.dataOutput = true;
                //addMsg(r2ad_vad_start, unit.m_pMem_vbv3->m_pWordInfo->pWordContent_UTF8);
                if (cmd) {
                    siren_printf(SIREN_INFO, "vad output with awake pre");
                    addMsg(r2ad_vad_start, unit.m_pMmem_bf->getinfo_sl());

                } else {
                    state.canceled = true;
                    unit.m_pMem_cod->pause();
                }
            } else if (state.asr && !state.awke) {
                float SlInfo[3] ;
                unit.m_pMem_vbv3->GetRealSl(36, SlInfo);
                if (!unit.m_pMmem_bf->check(SlInfo[0] , SlInfo[1])) {
                     siren_printf(SIREN_INFO, "SL    prev  %f  curr  %f", SlInfo[0], SlInfo[1]);
                }
                if (!config.alg_config.alg_vad_enable) {
                    unit.m_pMem_vad2->setvadendparam(2000);
                }
                siren_printf(SIREN_INFO, "vad start with !state.awke");
                state.dataOutput = true;
                addMsg(r2ad_vad_start, unit.m_pMmem_bf->getinfo_sl());
            }
        }

        if (state.dataOutput) {
            if (!state.canceled) {
                if (config.alg_config.alg_vad_enable) {
                    if (unit.m_pMem_cod->istoolong()) {
                        state.canceled = true;
                        siren_printf(SIREN_INFO, "Cancel since asr too long");
                    }
                }
                if (!state.awke && !asr) {
                    state.canceled = true;
                    siren_printf(SIREN_INFO, "Cancel since no asr flag");
                }

                if (state.canceled) {
                    unit.m_pMem_cod->pause();
                    addMsg(r2ad_vad_cancel, 0, nullptr);
                    if (!config.alg_config.alg_vad_enable) {
                        unit.m_pMem_vad2->setvadendparam(-1);
                        state.vadStart = false;
                        state.dataOutput = false;
                    }
                }
            }

            if (config.alg_config.alg_vad_enable) {
                if (state.canceled && state.awke) {
                    if (unit.m_pMem_cod->isneedresume()) {
                        siren_printf(SIREN_INFO, "reset output since asr too long");
                        state.canceled = false;
                        unit.m_pMem_cod->resume();
                        addMsg(r2ad_vad_start, unit.m_pMmem_bf->getinfo_sl());
                    }
                }
            }
            
            if (!state.canceled) {
                if (unit.m_pMem_cod->getdatalen() > 100) {
                    addMsg(r2ad_vad_data, unit.m_pMem_cod);
                }

                if (vad2 & r2vad_audio_end && unit.m_pMem_cod->getdatalen() > 0) {
                    addMsg(r2ad_vad_data, unit.m_pMem_cod);
                }
            }
        }
    }
    
    if (vad2 & r2vad_audio_end) {
        siren_printf(SIREN_INFO, "vad end");
        if (state.dataOutput && !state.canceled) {
            addMsg(r2ad_vad_end, 0, nullptr);
        }

        if (state.awke) {
            unit.m_pMem_cod->pause();
            unit.m_pMem_cod->resume();
        }

        state.awke = false;
        if (config.alg_config.alg_vt_enable) {
            state.asr = false;
            state.vadStart = false;
            state.dataOutput = false;
        }
        state.canceled = false;
    }
}



void SirenProcessorImpl::setState(r2v_sys_state awake_state) {
    if (awake_state == r2ssp_state_sleep && state.awke) {
        state.awke = false; 
    }

//    if (awake_state == r2ssp_state_awake && !config.alg_config.alg_vad_enable) {
//        siren_printf(SIREN_INFO, "set vad start");
//        state.asrMsgCheckFlag = 1;
//        state.dataOutput = true;
//        state.vadStart = true;    
//        state.forceStart = true;
//        state.canceled = false;
//        unit.m_pMem_cod->reset();
//    } else if (awake_state == r2ssp_state_sleep && !config.alg_config.alg_vad_enable) {
//        state.forceStart = false;
//        state.asrMsgCheckFlag = 0;
//    }
}


void SirenProcessorImpl::setSLSteer(float ho, float ver) {
    if (unit.m_pMmem_bf != nullptr) {
        unit.m_pMmem_bf->steer(ho, ver);
    }
}


void SirenProcessorImpl::getMsgs(r2ad_msg_block** &pMsgLst, int &iMsgNum) {
    pMsgLst = allocator.msgLst;
    iMsgNum = allocator.msgNumCurr;
    for (int i = 0; i < allocator.msgNumCurr; i++) {
        dumpMsg(allocator.msgLst[i]);
    }
}

void SirenProcessorImpl::addMsg(r2ad_msg msgid, int msgdatalen, const char *data) {
    if (allocator.msgNumCurr + 1 > allocator.msgNumTotal) {
        allocator.msgNumTotal = (allocator.msgNumCurr + 1) * 2;
        r2ad_msg_block** msglst = (r2ad_msg_block **)malloc(sizeof(r2ad_msg_block*) * allocator.msgNumTotal);
        memcpy(msglst, allocator.msgLst, sizeof(r2ad_msg_block *) * allocator.msgNumCurr);
        free(allocator.msgLst);
        allocator.msgLst = msglst;
    }

    r2ad_msg_block *msg = new r2ad_msg_block;
    msg->iMsgId = msgid;
    msg->iMsgDataLen = msgdatalen;

    if (msgdatalen > 0) {
        msg->pMsgData = new char[msgdatalen];
        memcpy(msg->pMsgData, data, sizeof(char) * msgdatalen);
    } else {
        msg->pMsgData = NULL;
    }

    allocator.msgLst[allocator.msgNumCurr] = msg;
    allocator.msgNumCurr++;
}

void SirenProcessorImpl::addMsg(r2ad_msg msgid, const char *sl) {
    addMsg(msgid, strlen(sl) + 1, sl);
}

void SirenProcessorImpl::addMsg(r2ad_msg msgid, r2mem_cod * cod) {
    char *data = nullptr;
    int len = 0;
    unit.m_pMem_cod->getdata2(data, len);
    addMsg(msgid, len, data);
}

void SirenProcessorImpl::dumpMsg(r2ad_msg_block * msg) {
    std::string dt = r2_getdatatime();
    switch(msg->iMsgId) {
    case r2ad_vad_start:
        siren_printf(SIREN_INFO, "vad start with msg %s", msg->pMsgData);
        if (state.asrMsgCheckFlag != 0) {
            siren_printf(SIREN_ERROR, "asrflag error");
        }
        state.asrMsgCheckFlag = 1;
        break;
    case r2ad_vad_data:
        siren_printf(SIREN_INFO, "vad data with len %d", msg->iMsgDataLen);
        if (state.asrMsgCheckFlag != 1) {
            siren_printf(SIREN_ERROR, "asrflag error");
        }
        break;
    case r2ad_vad_end:
        siren_printf(SIREN_INFO, "vad end");
        if (state.asrMsgCheckFlag != 1) {
            siren_printf(SIREN_ERROR, "asrflag error");
        }
        state.asrMsgCheckFlag = 0;
        break;
    case r2ad_vad_cancel:
        siren_printf(SIREN_INFO, "vad cancel");
        //if (state.asrMsgCheckFlag != 1) {
        //    siren_printf(SIREN_ERROR, "asrflag error");
        //}
        state.asrMsgCheckFlag = 0;
        break;
    case r2ad_awake_vad_start:
        siren_printf(SIREN_INFO, "awake vad start");
        break;
    case r2ad_awake_vad_data:
        siren_printf(SIREN_INFO, "awake vad data");
        break;
    case r2ad_awake_vad_end:
        siren_printf(SIREN_INFO, "awake vad end");
        break;
    case r2ad_awake_pre:
        siren_printf(SIREN_INFO, "awake pre");
        break;
    case r2ad_awake_nocmd:
        siren_printf(SIREN_INFO, "awake nocmd");
        break;
    case r2ad_awake_cmd:
        siren_printf(SIREN_INFO, "awake cmd");
        break;
    case r2ad_awake_cancel:
        siren_printf(SIREN_INFO, "awake cancel");
        break;
    case r2ad_sleep:
        siren_printf(SIREN_INFO, "sleep");
        break;
    case r2ad_hotword:
        siren_printf(SIREN_INFO, "hotword");
        break;
    case r2ad_sr:
        siren_printf(SIREN_INFO, "sr");
        break;
    case r2ad_debug_audio:
        siren_printf(SIREN_INFO, "debug audio");
        break;
    case r2ad_dirty:
        siren_printf(SIREN_INFO, "dirty");
        break;
    default:
        break;
    }
}

void SirenProcessorImpl::clearMsgLst() {
    for (int i = 0; i < allocator.msgNumCurr; i++) {
        delete [] allocator.msgLst[i]->pMsgData;
        delete allocator.msgLst[i];
    }

    allocator.msgNumCurr = 0;
}

void SirenProcessorImpl::resetASR() {
    if (state.dataOutput && !state.canceled) {
        addMsg(r2ad_vad_cancel, 0, nullptr);
    }

    state.asr = false;
    state.vadStart = false;
    if (config.alg_config.alg_vad_enable) {
        state.dataOutput = false;
    }
    state.canceled = false;
    state.awke = false;

    unit.m_pMem_vad2->reset();
    unit.m_pMem_cod->reset();
    unit.m_pMmem_bf->reset();
}

void SirenProcessorImpl::reset() {
    unit.m_pMem_vad2->reset();
    unit.m_pMem_cod->reset();

    state.asr = false;
    state.dataOutput = false;
    state.vadStart = false;
}

float SirenProcessorImpl::getLastFrameEnergy() {
    return unit.m_pMem_vbv3->GetEn_LastFrm();
}

float SirenProcessorImpl::getLastFrameThreshold() {
    return unit.m_pMem_vbv3->GetEn_Shield();
}

void SirenProcessorImpl::syncVTWord(std::vector<siren_vt_word> &words) {
    int defaultVTWordNum = config.alg_config.def_vt_configs.size();
    defaultVTWordNum += words.empty() ? 0 : words.size();
    siren_printf(SIREN_INFO, "sync vt word num: %d", defaultVTWordNum);
    micinfo.currentWordNum = defaultVTWordNum;
    if (micinfo.m_pWordLst != nullptr) {
        delete micinfo.m_pWordLst;
        micinfo.m_pWordLst = nullptr;
    }
    micinfo.m_pWordLst = new WordInfo[micinfo.currentWordNum];
    int i;

    //load default
    for (i = 0; i < (int)config.alg_config.def_vt_configs.size(); i++) {
        micinfo.m_pWordLst[i].iWordType = (WordType)config.alg_config.def_vt_configs[i].vt_type;
        strcpy(micinfo.m_pWordLst[i].pWordContent_UTF8,
               config.alg_config.def_vt_configs[i].vt_word.c_str());
        strcpy(micinfo.m_pWordLst[i].pWordContent_PHONE,
               config.alg_config.def_vt_configs[i].vt_phone.c_str());
        micinfo.m_pWordLst[i].fBlockAvgScore =
            config.alg_config.def_vt_configs[i].vt_avg_score;
        micinfo.m_pWordLst[i].fBlockMinScore =
            config.alg_config.def_vt_configs[i].vt_min_score;

        micinfo.m_pWordLst[i].bLeftSilDet =
            config.alg_config.def_vt_configs[i].vt_left_sil_det;
        micinfo.m_pWordLst[i].bRightSilDet =
            config.alg_config.def_vt_configs[i].vt_right_sil_det;

        micinfo.m_pWordLst[i].bRemoteAsrCheckWithAec =
            config.alg_config.def_vt_configs[i].vt_remote_check_with_aec;
        micinfo.m_pWordLst[i].bRemoteAsrCheckWithNoAec =
            config.alg_config.def_vt_configs[i].vt_remote_check_without_aec;

        micinfo.m_pWordLst[i].bLocalClassifyCheck =
            config.alg_config.def_vt_configs[i].vt_local_classify_check;
        micinfo.m_pWordLst[i].fClassifyShield =
            config.alg_config.def_vt_configs[i].vt_classify_shield;

        bool nnet_valid = false;
        if (!config.alg_config.def_vt_configs[i].vt_nnet_path.empty()) {
            strcpy(micinfo.m_pWordLst[i].pLocalClassifyNnetPath,
                   config.alg_config.def_vt_configs[i].vt_nnet_path.c_str());
            nnet_valid = true;
        } else {
            nnet_valid = false;
        }

        siren_printf(SIREN_INFO, "load word%d", i);
        siren_printf(SIREN_INFO, "word content: %s", micinfo.m_pWordLst[i].pWordContent_UTF8);
        siren_printf(SIREN_INFO, "phone: %s", micinfo.m_pWordLst[i].pWordContent_PHONE);
        siren_printf(SIREN_INFO, "block avg score %f", micinfo.m_pWordLst[i].fBlockAvgScore);
        siren_printf(SIREN_INFO, "block min score %f", micinfo.m_pWordLst[i].fBlockMinScore);
        siren_printf(SIREN_INFO, "left sil det %s", micinfo.m_pWordLst[i].bLeftSilDet ? "true" : "false");
        siren_printf(SIREN_INFO, "right sil det %s", micinfo.m_pWordLst[i].bRightSilDet ? "true" : "false");
        siren_printf(SIREN_INFO, "remote asr check with aec %s",
                     micinfo.m_pWordLst[i].bRemoteAsrCheckWithAec ? "true" : "false");
        siren_printf(SIREN_INFO, "remote asr check without aec %s",
                     micinfo.m_pWordLst[i].bRemoteAsrCheckWithNoAec ? "true" : "false");
        siren_printf(SIREN_INFO, "local classify check %s", micinfo.m_pWordLst[i].bLocalClassifyCheck ? "true" : "false");
        siren_printf(SIREN_INFO, "classify shield %f", micinfo.m_pWordLst[i].fClassifyShield);

        if (nnet_valid) {
            siren_printf(SIREN_INFO, "nnet valid path %s",
                         micinfo.m_pWordLst[i].pLocalClassifyNnetPath);
        } else {
            siren_printf(SIREN_INFO, "neet invalid maybe useless");
        }
    }

    for (siren_vt_word word : words) {
        micinfo.m_pWordLst[i].iWordType = (WordType)word.vt_type;
        strcpy(micinfo.m_pWordLst[i].pWordContent_UTF8, word.vt_word.c_str());
        strcpy(micinfo.m_pWordLst[i].pWordContent_PHONE, word.vt_phone.c_str());

        micinfo.m_pWordLst[i].fBlockAvgScore = word.alg_config.vt_block_avg_score;
        micinfo.m_pWordLst[i].fBlockMinScore = word.alg_config.vt_block_min_score;

        micinfo.m_pWordLst[i].bLeftSilDet = word.alg_config.vt_left_sil_det;
        micinfo.m_pWordLst[i].bRightSilDet = word.alg_config.vt_right_sil_det;

        micinfo.m_pWordLst[i].bRemoteAsrCheckWithAec = word.alg_config.vt_remote_check_with_aec;
        micinfo.m_pWordLst[i].bRemoteAsrCheckWithNoAec = word.alg_config.vt_remote_check_without_aec;

        micinfo.m_pWordLst[i].bLocalClassifyCheck = word.alg_config.vt_local_classify_check;
        micinfo.m_pWordLst[i].fClassifyShield = word.alg_config.vt_classify_shield;

        bool nnet_valid = false;
        if (!word.alg_config.nnet_path.empty()) {
            strcpy(micinfo.m_pWordLst[i].pLocalClassifyNnetPath, word.alg_config.nnet_path.c_str());
            nnet_valid = true;
        } else {
            nnet_valid = false;
        }

        siren_printf(SIREN_INFO, "load word%d", i);
        siren_printf(SIREN_INFO, "word content: %s", micinfo.m_pWordLst[i].pWordContent_UTF8);
        siren_printf(SIREN_INFO, "phone: %s", micinfo.m_pWordLst[i].pWordContent_PHONE);
        siren_printf(SIREN_INFO, "block avg score %f", micinfo.m_pWordLst[i].fBlockAvgScore);
        siren_printf(SIREN_INFO, "block min score %f", micinfo.m_pWordLst[i].fBlockMinScore);
        siren_printf(SIREN_INFO, "left sil det %s", micinfo.m_pWordLst[i].bLeftSilDet ? "true" : "false");
        siren_printf(SIREN_INFO, "right sil det %s", micinfo.m_pWordLst[i].bRightSilDet ? "true" : "false");
        siren_printf(SIREN_INFO, "remote asr check with aec %s",
                     micinfo.m_pWordLst[i].bRemoteAsrCheckWithAec ? "true" : "false");
        siren_printf(SIREN_INFO, "remote asr check without aec %s",
                     micinfo.m_pWordLst[i].bRemoteAsrCheckWithNoAec ? "true" : "false");
        siren_printf(SIREN_INFO, "local classify check %s", micinfo.m_pWordLst[i].bLocalClassifyCheck ? "true" : "false");
        siren_printf(SIREN_INFO, "classify shield %f", micinfo.m_pWordLst[i].fClassifyShield);

        if (nnet_valid) {
            siren_printf(SIREN_INFO, "nnet valid path %s",
                         micinfo.m_pWordLst[i].pLocalClassifyNnetPath);
        } else {
            siren_printf(SIREN_INFO, "neet invalid maybe useless");
        }
        i++;
    }

    siren_printf(SIREN_INFO, "sync %d words", i);
    unit.m_pMem_vbv3->SetWords(micinfo.m_pWordLst, micinfo.currentWordNum);
}

int SirenProcessorImpl::getVTInfo(std::string &vt_word, int &start, int &end, float &energy) {
    if (unit.m_pMem_vbv3 != nullptr && unit.m_pMem_vbv3->m_pWordInfo != nullptr) {
        vt_word = unit.m_pMem_vbv3->m_pWordInfo->pWordContent_UTF8;
        //end from back to front
        //start = unit.m_pMem_vbv3->m_pWordDetInfo->iWordPos_Start;
        start = 20 * state.frmSize;
        //start
        end = start + unit.m_pMem_vbv3->m_pWordDetInfo->iWordPos_Start - unit.m_pMem_vbv3->m_pWordDetInfo->iWordPos_End;
        energy = unit.m_pMem_vbv3->m_pWordDetInfo->fEnergy;
        return 0;
    } else {
        siren_printf(SIREN_ERROR, "failed since: %d %d",
                     unit.m_pMem_vbv3 == nullptr,
                     unit.m_pMem_vbv3->m_pWordInfo == nullptr);
        return -1;
    }

}

}
