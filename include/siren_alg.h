#ifndef SIREN_ALG_H_
#define SIREN_ALG_H_


// USE LEGACY INTERFACE
//TODO: use new interface instead

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
    int block;
    int hasSL;
    int hasVoice;
    double sl;
    char *data;
    void release() {
        delete this;
    }
} ;

struct PreprocessVoicePackage {
    int msg;
    int aec;
    int size;
    char *data;
    void release() {
        delete this;
    }
};

class SirenAudioPreProcessor {
public:
    SirenAudioPreProcessor(int size, SirenConfig &config_):
        config(config_),
        preprocessBuffSize(size) {}
    ~SirenAudioPreProcessor() = default;
    void preprocess(char *rawBuffer, PreprocessVoicePackage **voicePackage);    
    siren_status_t init();
    siren_status_t destroy();
private:
    SirenConfig &config;
    int preprocessBuffSize;
    int currentLan;
    
    r2ad1_htask ad1;
    bool ad1Init;
};

class SirenAudioVBVProcessor {
public:
    SirenAudioVBVProcessor(SirenConfig &config_) :
        config(config_),
        r2v_state(r2ssp_state_sleep) {}    
    ~SirenAudioVBVProcessor() = default;
    void process(PreprocessVoicePackage *voicePackage, ProcessedVoiceResult **result);
     

    siren_status_t init();
    siren_status_t destroy();

private:
    SirenConfig &config;

    //asr state
    r2v_sys_state r2v_state;
    
    //current lan
    int currentLan;    

    //legacy processor
    r2ad2_htask ad2;
    bool ad2Init;
};

}

#endif
