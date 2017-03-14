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

struct PreprocessVociePackage {
    int msg;
    int aec;
    int size;
    char *data;
    void release() {
        delete this;
    }
};

class SirenAudioProcessor {
public:
    SirenAudioProcessor(int size) :
        preprocessBuffSize(size) {}
    
    ~SirenAudioProcessor() = default;
    void process(PreprocessVociePackage *voicePackage, ProcessedVoiceResult **result);

    siren_status_t init(SirenConfig &config);
    siren_status_t destroy();

private:
    int preprocessBuffSize;

    //asr state
    r2v_sys_state r2v_state;
    
    //current lan
    int currentLan;    

    //legacy processor
    r2ad1_htask ad1;
    bool ad1Init;

    r2ad2_htask ad2;
    bool ad2Init;

    r2ad3_htask ad3;
    bool ad3Init;
};

}

#endif
