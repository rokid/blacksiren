#include <cstdlib>
#include <cstring>
#include <cassert>

#include "neo/siren_mic.h"
#include "sutils.h"
#include "legacy/r2math.h"

namespace BlackSiren {

static void validFormat(const PCMFormat &format) {
    switch (format.type) {
    case in_int_24: {
        assert(format.frameLen % (sizeof(type_int24) * format.channelNum) == 0);
    }
    break;
    case in_int_32:
    case in_int_32_10: {
        assert(format.frameLen % (sizeof(int) * format.channelNum) == 0);
    }
    break;
    case in_float_32: {
        assert(format.frameLen % (sizeof(float) * format.channelNum) == 0);
    }
    break;
    default: {
        assert(false || "not support such format");
    }
    }
}

void formatFromNormalPCM(float *formatPCM, char *pcm, const PCMFormat &format) {
    assert(formatPCM != nullptr);
    assert(pcm != nullptr);
    validFormat(format);
    int perChannelLen = 0;

    switch(format.type) {
    case in_int_24: {
        type_int24 *data = (type_int24 *) pcm;
        perChannelLen = format.frameLen / (sizeof(type_int24) * format.channelNum);
        for (int j = 0; j < format.channelNum; j++) {
            int id = format.channelList[j];
            for (int k = 0; k < perChannelLen; k++) {
                formatPCM[perChannelLen * id + k] = (float)data[k * format.channelNum + j].toint() / 4.0f;
            }
        }
    }
    break;
    case in_int_32_10: {
        int *data = (int *)pcm;
        perChannelLen = format.frameLen / (sizeof(int) * format.channelNum);
        for (int j = 0; j < format.channelNum; j++) {
            int id = format.channelList[j];
            for (int k = 0; k < perChannelLen; k++) {
                formatPCM[perChannelLen * id + k] = (float)data[k * format.channelNum + j] / 4.0f;
            }
        }
    }
    break;
    case in_int_32: {
        int *data = (int *)pcm;
        perChannelLen = format.frameLen / (sizeof(int) * format.channelNum);
        for (int j = 0; j < format.channelNum; j++) {
            int id = format.channelList[j];
            for (int k = 0; k < perChannelLen; k++) {
                formatPCM[perChannelLen * id + k] = (float)data[k * format.channelNum + j] / 1024.0f;
            }
        }
    }
    break;
    case in_float_32: {
        float *data = (float *)pcm;
        perChannelLen = format.frameLen / (sizeof(float) * format.channelNum);
        for (int j = 0; j < format.channelNum; j++) {
            int id = format.channelList[j];
            for (int k = 0; k < perChannelLen; k++) {
                formatPCM[perChannelLen * id + k] = (float)data[k * format.channelNum + j];
            }
        }
    }
    break;
    default: {
        siren_printf(SIREN_WARNING, "not support this type now");
    }
    }
}

mic_state MicStatus::putData(float **formatData, int len) {
    if (formatData == nullptr || len <= 0) {
        siren_printf(SIREN_ERROR, "check buffer or len invalid");
        return BUFFER_ERROR;    
    }

    int cur = 0;
    int needLen = maxCheckLen - currentLen;
    
    if (needLen <= 0) {
        currentLen = maxCheckLen;
        return BUFFER_FINISH;
    }
    
    if (len > needLen) {
        cur = needLen;
    } else {
        cur = len;
    }

    for (int i = 0; i < micNum; i++) {
        int id = format.channelList[i];
        for (int j = 0; j < cur; j++) {
            check[i] += formatData[id][j];
            checkVar[i] += formatData[id][j] * formatData[id][j];
        }
    }
    currentLen += cur;
    if (currentLen >= maxCheckLen) {
        currentLen = maxCheckLen;
        for (int i = 0; i < micNum; i++) {
            check[i] = check[i] / currentLen;
            checkVar[i] = sqrt(checkVar[i] / currentLen - (check[i] * check[i] + 0.1f));
            siren_printf(SIREN_INFO, "----Calculate MIC AVG %d, %f", format.channelList[i], checkVar[i]);
        }

        for (int i = 0; i < micNum; i++) {
            if (checkVar[i] > 200000) {
                siren_printf(SIREN_ERROR, "-----MIC ERROR %d-------", format.channelList[i]);
                micOK[i] = MIC_TEST_FAILED;
            } else {
                micOK[i] = MIC_TEST_OK;
            }
        }
        return BUFFER_FINISH;
    }

    return BUFFER_AGAIN;
} 


}
