#ifndef SIREN_ALG_H_
#define SIREN_ALG_H_
#include <functional>

// USE LEGACY INTERFACE
//TODO: use new interface instead

#include <vector>
#include <memory>
#include <map>

#include "legacy/r2ad1.h"
#include "legacy/r2ad2.h"
#include "legacy/r2ad3.h"

#include "siren.h"
#include "siren_config.h"
#include "common.h"
//#include "siren_preprocessor.h"
//#include "siren_processor.h"

namespace BlackSiren {

class SirenPreprocessorImpl;
class SirenProcessorImpl;
struct ProcessedVoiceResult {
    int size;
    int debug;
    int prop;
    int hasSL;
    int hasVoice;
    int hasVT;
    double sl;
    double background_energy;
    double background_threshold;

    int start;
    int end;
    float vt_energy;
    char *data;
} ;

struct PreprocessVoicePackage {
    int msg;
    int aec;
    int size;
    int padding;
    char *data;
} ;

ProcessedVoiceResult *allocateProcessedVoiceResult(int size, int debug, int prop, int start, int end,
        int hasSL, int hasVoice, int hasVT, double sl, double energy, double threshold, float vt_energy);
PreprocessVoicePackage *allocatePreprocessVoicePackage(int msg, int aec, int size);

typedef void (*on_state_changed)(int current);
class SirenAudioPreProcessor {
public:
    SirenAudioPreProcessor(int size, SirenConfig &config_):
        config(config_),
        frameSize(size),
        averageDelay(0) {}
    ~SirenAudioPreProcessor() = default;
    void preprocess(char *rawBuffer, PreprocessVoicePackage **voicePackage);
    siren_status_t init();
    siren_status_t destroy();
private:
    SirenConfig &config;
    int frameSize;
    int currentLan;
    int averageDelay;

#ifdef CONFIG_USE_AD1
    r2ad1_htask ad1;
#else
    std::shared_ptr<SirenPreprocessorImpl> pImpl;
#endif

#ifdef CONFIG_USE_AD1
    bool ad1Init = false;
#else
    bool preprocessorInit = false;
#endif

};

class SirenAudioVBVProcessor {
public:
    SirenAudioVBVProcessor(SirenConfig &config_, std::function<void(int)>& stateCallback_) :
        stateCallback(stateCallback_),
        config(config_),
        r2v_state(r2ssp_state_sleep)
    {}
    ~SirenAudioVBVProcessor() = default;
    int process(PreprocessVoicePackage *voicePackage, std::vector<ProcessedVoiceResult*> &result);

    bool hasSlInfo(int prop);
    bool hasVoice(int prop);
    bool hasVTInfo(int prop, char *data);

    void setSysState(int state, bool shouldCallback);
    void setSysSteer(float ho, float ver);
    void syncVTWord(std::vector<siren_vt_word> &words);
    siren_status_t init();
    siren_status_t destroy();

private:
    std::function<void(int)>& stateCallback;
    SirenConfig &config;
    //asr state
    r2v_sys_state r2v_state;

    //current lan
    int currentLan;

#ifdef CONFIG_USE_AD2
    //legacy processor
    r2ad2_htask ad2;
    bool ad2Init;
#else
    std::shared_ptr<SirenProcessorImpl> pImpl;
    bool processorInit = false;
#endif

};

struct PhonemeEle {
    int num;
    std::string head;
    std::vector<std::string> contents;

    std::string initials;
    std::string finals;
    std::string result;

    bool spliteHead(std::string &head, std::string &line);
    void genResult();
};

class SirenPhonemeGen {
public:
    void loadPhoneme();
    bool pinyin2Phoneme(const char *pinyin, std::string &result);

    ~SirenPhonemeGen() {
        phonemeMaps.clear();
    }

private:
    bool getline(char *line);
    
    int offset = 0;
    std::map<std::string, PhonemeEle> phonemeMaps;

};

}

#endif
