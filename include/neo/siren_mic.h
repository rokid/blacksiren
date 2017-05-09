#ifndef SIREN_MIC_H_
#define SIREN_MIC_H_

#include <vector>
#include <cstring>

namespace BlackSiren {

enum in_type {
    in_int_24 = 1,
    in_int_32,
    in_int_32_10,
    in_float_32,
    in_int_16,
    in_int_8
};


struct type_int24 {
    unsigned char internal[3];
    int toint() {
        int top = ((internal[2] & 0x80) == 0) ? 0 : -1;
        return ((internal[0] & 0xff) | ((internal[1] & 0xff) << 8) | (( internal[2] & 0xff) < 16) | ((top & 0xff) << 24));
    }

};

struct PCMFormat {
    //in byte
    int frameLen;
    in_type type;
    int sampleRate;
    int channelNum;
    std::vector<int> channelList;
};

void formatFromNormalPCM(float *formatPCM, char *pcm, const PCMFormat &format);

enum mic_state {
    MIC_NOT_TEST,
    MIC_TEST_OK,
    MIC_TEST_FAILED,
    MIC_ERROR,

    BUFFER_AGAIN,
    BUFFER_FINISH,
    BUFFER_ERROR,
};

class MicStatus {
public:
    MicStatus(int mic_, int maxCheckLen_, PCMFormat &format_) : micNum(mic_)
        ,   maxCheckLen(maxCheckLen_)
        ,   currentLen(0)
        ,   format(format_) {
        check = new float[mic_];
        checkVar = new double[mic_];
        micOK = new mic_state[mic_];
        for (int i = 0; i < mic_; i++) {
            micOK[i] = MIC_NOT_TEST;
        }    
    }

    ~MicStatus() {
        delete check;
        delete checkVar;
        delete micOK;
    }

    mic_state getMicState(int which) {
        if (which >= micNum) {
            return MIC_ERROR; 
        }

        return micOK[which];
    }

    mic_state putData(float **formatData, int len);
    MicStatus(const MicStatus &status) = delete;
    void reset() {
        memset(check, 0, sizeof(float) * micNum);
        memset(checkVar, 0, sizeof(double) * micNum);
        memset(micOK, 0, sizeof(mic_state) * micNum);
        currentLen = 0;
    }
private:
    int micNum;
    int maxCheckLen;
    int currentLen;

    float *check;
    double *checkVar;
    mic_state *micOK;
    PCMFormat &format;
};



}

#endif
