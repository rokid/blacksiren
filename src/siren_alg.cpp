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
#include "siren_preprocessor.h"
#include "siren_processor.h"
#include "phoneme.h"

namespace BlackSiren {

PreprocessVoicePackage* allocatePreprocessVoicePackage(int msg, int aec, int size) {
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

ProcessedVoiceResult* allocateProcessedVoiceResult(int size, int debug, int prop, int start, int end,
        int hasSL, int hasV, int hasVT, double sl, double energy, double threshold, float vt_energy) {
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
    pvr->background_energy = energy;
    pvr->background_threshold = threshold;
    pvr->start = start;
    pvr->end = end;
    pvr->hasVT = hasVT;
    pvr->vt_energy = vt_energy;

    return pvr;
}

siren_status_t SirenAudioPreProcessor::init() {
    //TODO use config file
    currentLan = config.alg_config.alg_lan;
    int status = 0;
#ifdef CONFIG_USE_AD1
    siren_printf(SIREN_INFO, "use ad1");
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
#else
    siren_printf(SIREN_INFO, "use preprocessor!");
    pImpl = std::make_shared<SirenPreprocessorImpl>(config);
    status = pImpl->init();
    if (status != 0) {
        siren_printf(SIREN_ERROR, "init preprocessor impl");
        preprocessorInit = false;
        return SIREN_STATUS_ERROR;
    } else {
        preprocessorInit = true;
    }
#endif

    return SIREN_STATUS_OK;
}


void SirenAudioPreProcessor::preprocess(char *rawBuffer, PreprocessVoicePackage **voicePackage) {
#ifdef CONFIG_USE_AD1
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
        //siren_printf(SIREN_ERROR, "r2ad1_getdatalen return 0");
        return;
    }

    PreprocessVoicePackage *vp = allocatePreprocessVoicePackage(SIREN_REQUEST_MSG_DATA_PROCESS, aec, len);
    if (vp == nullptr) {
        siren_printf(SIREN_ERROR, "allocatePreprocessVoicePackage FAILED");
        return;
    }

    r2ad1_getdata(ad1, vp->data, len);
    *voicePackage = vp;
#else
    if (!preprocessorInit) {
        siren_printf(SIREN_ERROR, "preprocessor not init");
        return;
    }

    int aec = 0;
    if (rawBuffer == nullptr) {
        siren_printf(SIREN_ERROR, "raw buffer is null");
        return;
    }

    aec = pImpl->processData(rawBuffer, frameSize);
    int len = pImpl->getResultLen();
    if (len <= 0) {
        return;
    }

    PreprocessVoicePackage *vp = allocatePreprocessVoicePackage(SIREN_REQUEST_MSG_DATA_PROCESS, aec, len);
    if (vp == nullptr) {
        siren_printf(SIREN_ERROR, "allocatePreprocessVoicePackage FAILED");
        return;
    }

    pImpl->getResult(vp->data, len);
    *voicePackage = vp;
#endif
}

siren_status_t SirenAudioPreProcessor::destroy() {
#ifdef CONFIG_USE_AD1
    if (ad1Init) {
        r2ad1_sysexit();
    }
#else
    if (preprocessorInit) {
        pImpl->destroy();
    }
#endif

    return SIREN_STATUS_OK;
}


siren_status_t SirenAudioVBVProcessor::init() {
    //TODO use config file
    currentLan = config.alg_config.alg_lan;
    int status = 0;
#ifdef CONFIG_USE_AD2
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

    r2ad2_steer(ad2, 180.0, 0.0);
#else
    siren_printf(SIREN_INFO, "use processor!");
    pImpl = std::make_shared<SirenProcessorImpl>(config);
    status = pImpl->init();
    if (status != 0) {
        siren_printf(SIREN_ERROR, "init processor impl");
        processorInit = false;
        return SIREN_STATUS_ERROR;
    } else {
        processorInit = true;
    }
#endif
    return SIREN_STATUS_OK;
}

siren_status_t SirenAudioVBVProcessor::destroy() {
#ifdef CONFIG_USE_AD2
    if (ad2Init) {
        r2ad2_sysexit();
    }
#else
    if (processorInit) {
        pImpl->destroy();
    }

#endif
    return SIREN_STATUS_OK;
}

bool SirenAudioVBVProcessor::hasSlInfo(int prop) {
    if (prop == r2ad_awake_pre ||
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

bool SirenAudioVBVProcessor::hasVTInfo(int prop, char *data) {
    if (prop == r2ad_debug_audio) {
        return true;
    } else {
        return false;
    }
}

void SirenAudioVBVProcessor::setSysState(int state, bool shouldCallback) {
    r2v_state = (r2v_sys_state)state;
    if (shouldCallback) {
        stateCallback((int)r2v_state);
    }
}

void SirenAudioVBVProcessor::setSysSteer(float ho, float ver) {
#ifdef CONFIG_USE_AD2
    r2ad2_steer (ad2, ho, ver);
#else
    pImpl->setSLSteer(ho, ver);
#endif
}


void SirenAudioVBVProcessor::syncVTWord(std::vector<siren_vt_word> &words) {
    if (words.empty()) {
        siren_printf(SIREN_INFO, "sync vt words with empty words");
        return;
    }
#ifdef CONFIG_USE_AD2
    siren_printf(SIREN_INFO, "not support in ad2 version");
    return;
#else
    pImpl->syncVTWord(words);
#endif

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

#ifdef CONFIG_USE_AD2
    r2ad2_putaudiodata2(ad2, voicePackage->data, voicePackage->size, voicePackage->aec, 1, asrFlag, asrFlag);
    r2ad_msg_block **ppR2ad_msg_block = nullptr;
    int len = 0;
    int prop = 0;
    int block_num = 0;
    int hasSL = 0;
    int hasV = 0;
    int hasVT = 0;
    int debug = 0;
    int start = 0;
    int end = 0;
    float vt_energy = 0.0f;
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
            pProcessedVoiceResult = allocateProcessedVoiceResult(len, debug, prop, start, end,
                                    hasSL, hasV, hasVT, sl, energy, threshold, vt_energy);
            memcpy(pProcessedVoiceResult->data, ppR2ad_msg_block[i]->pMsgData, len);
        } else {
            hasV = 0;
            energy = 0.0;
            threshold = 0.0;
            pProcessedVoiceResult = allocateProcessedVoiceResult(0, debug, prop, start, end,
                                    hasSL, hasV, hasVT, sl, energy, threshold, vt_energy);
        }

        result.push_back(pProcessedVoiceResult);
    }

#else
    // r2ad2_putaudiodata2(ad2, voicePackage->data, voicePackage->size, voicePackage->aec, 1, asrFlag, asrFlag);
    pImpl->process(voicePackage->data, voicePackage->size, voicePackage->aec, 1, asrFlag, asrFlag, 0);
    r2ad_msg_block **ppR2ad_msg_block = nullptr;
    int len = 0;
    int prop = 0;
    int block_num = 0;
    int hasSL = 0;
    int hasV = 0;
    int hasVT = 0;
    int debug = 0;
    int start = 0;
    int end = 0;
    float vt_energy = 0.0f;
    double energy = 0.0;
    double threshold = 0.0;
    double sl = 0.0;
    std::string vt_word;

    ProcessedVoiceResult *pProcessedVoiceResult = nullptr;
    //r2ad2_getmsg2(ad2, &ppR2ad_msg_block, &block_num);
    pImpl->getMsgs(ppR2ad_msg_block, block_num);
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

        if (hasVTInfo(prop, ppR2ad_msg_block[i]->pMsgData)) {
            //end and start is reverse
            if (0 != pImpl->getVTInfo(vt_word, end, start, vt_energy)) {
                siren_printf(SIREN_ERROR, "wtf hasVTInfo but get VTInfo failed");
            } else {
                hasVT = 1;
                siren_printf(SIREN_INFO, "vt word with %s [%d, %d]%f", vt_word.c_str(), start, end, vt_energy);
            }
        }


        if (len != 0 && hasVoice(prop)) {
            hasV = 1;
            energy = static_cast<double>(pImpl->getLastFrameEnergy());
            threshold = static_cast<double>(pImpl->getLastFrameThreshold());
            pProcessedVoiceResult = allocateProcessedVoiceResult(len, debug, prop, start, end,
                                    hasSL, hasV, hasVT, sl, energy, threshold, vt_energy);
            memcpy(pProcessedVoiceResult->data, ppR2ad_msg_block[i]->pMsgData, len);
        } else if (hasVT == 1) {
            hasV = 0;
            len = vt_word.size() + 1;
            energy = static_cast<double>(pImpl->getLastFrameEnergy());
            threshold = static_cast<double>(pImpl->getLastFrameThreshold());
            pProcessedVoiceResult = allocateProcessedVoiceResult(len, debug, prop, start, end,
                                    hasSL, hasV, hasVT, sl, energy, threshold, vt_energy);
            memcpy(pProcessedVoiceResult->data, vt_word.c_str(), len);
        } else {
            hasV = 0;
            energy = 0.0;
            threshold = 0.0;
            pProcessedVoiceResult = allocateProcessedVoiceResult(len, debug, prop, start, end,
                                    hasSL, hasV, hasVT, sl, energy, threshold, vt_energy);
        }

        result.push_back(pProcessedVoiceResult);
    }



#endif
    return block_num;
}

bool PhonemeEle::spliteHead(std::string &head, std::string &line) {
    const char *p = line.c_str();
    const char *q = p;

    bool findHead = false;
    while (1) {
        if (*p == '\0') {
            break;
        }

        if (*p == '_') {
            head.assign(q, p - q);
            findHead = true;
            break;
        }
        p++;
    }

    return findHead;

}

void PhonemeEle::genResult() {
    if (contents.empty()) {
        return;
    }

    std::string initialHead;
    bool find = spliteHead(initialHead, contents[0]);

    if (find) {
        initials = initialHead + "|" + contents[0];
    } else {
        initials = contents[0];
    }

    if (contents.size() == 1) {
        result = initials;
        //siren_printf(SIREN_INFO, "initials = %s finals = ", initials.c_str());
        return;
    }

    for (std::size_t i = 1; i < contents.size(); i++) {
        std::string head;
        std::string intermid;
        find = spliteHead(head, contents[i]);

        if (find) {
            intermid = head + "|" + contents[i] + " ";
        } else {
            intermid = contents[i] + " ";
        }

        finals.append(intermid);
    }

    finals = finals.substr(0, finals.size() - 1);
    //siren_printf(SIREN_INFO, "initials = %s, finals = %s", initials.c_str(), finals.c_str());

    result.append(initials).append("|# ").append(finals).append("|#");
    //siren_printf(SIREN_INFO, "result = %s", result.c_str());
}

bool SirenPhonemeGen::getline(char *line) {
    int i = 0;
    while (1) {
        if (PHONEME[offset] == '\n') {
            if (PHONEME[offset + 1] == '\0') {
                siren_printf(SIREN_INFO, "read end");
                return true;
            }
            offset++;
            return false;
        }

        line[i] = PHONEME[offset];
        i++;
        offset++;
    }

    return false;
}

void SirenPhonemeGen::loadPhoneme() {
    bool eof = false;
    while (!eof) {
        PhonemeEle line_index;
        int i = 0;
        int j = 0;
        int num = 0;
        char line[1024] = {0};
        bool eol = false;
        eof = getline(line);
        //siren_printf(SIREN_INFO, "line=%s", line);
        if (line[0] == ' ' || line[0] == '\0') {
            break;
        }

        //split ' '
        while (1) {
            if (line[i] == ' ') {
                break;
            }
            i++;
        }

        line_index.head.assign(line, i);
        //siren_printf(SIREN_INFO, "read head=%s", line_index.head.c_str());

        i++;
        while (1) {
            j = i;
            while (1) {
                if (line[j] == ' ') {
                    break;
                }

                if (line[j] == '\0') {
                    eol = true;
                    break;
                }

                j++;
            }

            std::string content;
            content.assign(line + i, j - i);
            line_index.contents.push_back(content);

            j++;
            i = j;
            num++;
            if (eol) {
                break;
            }
        }
        line_index.num = num;
        if (line_index.num != (int)line_index.contents.size()) {
            siren_printf(SIREN_ERROR, "wtf index num not equals to size");
        }
        line_index.genResult();
        phonemeMaps.insert(std::make_pair(line_index.head, line_index));
    }

    offset = 0;
}

bool SirenPhonemeGen::pinyin2Phoneme(const char *pinyin, std::string &result) {
    if (pinyin == nullptr) {
        siren_printf(SIREN_ERROR, "empty pinyin");
        return false;
    }
    
    std::vector<std::string> targets;
    const char *p = pinyin;
    const char *q = pinyin;
    while (1) {
        if (*p == '\0') {
            printf("read end\n");
            break;
        }

        if (!std::isalnum((int)*p)) {
            printf("contains bad pinyin\n");
            return false;
        }

        if (std::isdigit((int)*p)) {
            std::string cut;
            cut.assign(q, p - q + 1);
            targets.push_back(cut);
            p++;
            q = p;
        } else {
            p++;
        }
    }

    if (targets.empty()) {
        siren_printf(SIREN_INFO, "pinyin target is empty");
        return false;
    }

    std::string finalResult;
    for (std::string str : targets) {
        if (str.empty()) {
            siren_printf(SIREN_ERROR, "wtf empty target pinyin\n");
            return false;
        }

        std::map<std::string, PhonemeEle>::iterator iter = phonemeMaps.find(str);
        if (iter == phonemeMaps.end()) {
            siren_printf(SIREN_ERROR, "cannot find %s", str.c_str());
            return false;
        }

        finalResult.append(iter->second.result).append(" ");
    }

    finalResult = finalResult.substr(0, finalResult.size() - 1);
    //siren_printf(SIREN_INFO, "%s to phoneme is %s", pinyin, finalResult.c_str());
    result = finalResult;
    
    return true;
}



}
