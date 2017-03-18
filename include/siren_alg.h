#ifndef SIREN_ALG_H_
#define SIREN_ALG_H_
#include <functional>

// USE LEGACY INTERFACE
//TODO: use new interface instead

#include <vector>

#include "legacy/r2ad1.h"
#include "legacy/r2ad2.h"
#include "legacy/r2ad3.h"

#include "siren.h"
#include "siren_config.h"

namespace BlackSiren {

struct ProcessedVoiceResult {
    int size;
    int debug;
    int prop;
    int hasSL;
    int hasVoice;
    double sl;
    char *data;
    double energy;
    double threshold;
    static ProcessedVoiceResult *allocateProcessedVoiceResult(int size, int debug, int prop,
            int hasSL, int hasVoice, double sl, double energy, double threshold);
    void release() {
        delete (char *)this;
    }
} ;

struct PreprocessVoicePackage {
    int msg;
    int aec;
    int size;
    char *data;
    static PreprocessVoicePackage *allocatePreprocessVoicePackage(int msg, int aec, int size);
    void release() {
        delete (char *)this;
    }
};

typedef void (*on_state_changed)(int current);

class SirenAudioPreProcessor {
public:
    SirenAudioPreProcessor(int size, SirenConfig &config_):
        config(config_),
        frameSize(size),
        averageDelay(0){}
    ~SirenAudioPreProcessor() = default;
    void preprocess(char *rawBuffer, PreprocessVoicePackage **voicePackage);    
    siren_status_t init();
    siren_status_t destroy();
private:
    SirenConfig &config;
    int frameSize;
    int currentLan;
    int averageDelay;

    r2ad1_htask ad1;
    bool ad1Init;
};

class SirenAudioVBVProcessor {
public:
    SirenAudioVBVProcessor(SirenConfig &config_, std::function<void(int)>& stateCallback_) :
        stateCallback(stateCallback_),
        config(config_),
        r2v_state(r2ssp_state_sleep),
        setState(false) {}    
    ~SirenAudioVBVProcessor() = default;
    int process(PreprocessVoicePackage *voicePackage, std::vector<ProcessedVoiceResult*> **result);

    bool hasSlInfo(int prop);
    bool hasVoice(int prop);

    void setSysState(int state);
    void setSysSteer(float ho, float ver);

    siren_status_t init();
    siren_status_t destroy();

private:
    std::function<void(int)>& stateCallback;
    SirenConfig &config;
    //asr state
    r2v_sys_state r2v_state;
    bool setState;

    //current lan
    int currentLan;    
    //legacy processor
    r2ad2_htask ad2;
    bool ad2Init;
};

}

#endif
