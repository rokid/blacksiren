#ifndef SIREN_PROCESSOR_H_
#define SIREN_PROCESSOR_H_


#include "siren_config.h"
#include "common.h"
#include "siren_alg_legacy_helper.h"
#include "sutils.h"
#include "siren_alg.h"

#include <fstream>
#include <vector>
#include "legacy/r2math.h"
#include "legacy/r2mem_i.h"
#include "legacy/r2mem_cod.h"
#include "legacy/r2mem_vad2.h"
#include "legacy/r2mem_vbv3.h"
#include "legacy/r2mem_bf.h"

namespace BlackSiren {
class SirenProcessorImpl {
public:
    SirenProcessorImpl(SirenConfig &config_) : config(config_) {
        std::string bfpath(config.debug_config.recording_path);
        bfpath.append("/bf_debug.pcm");
        bfRecordingPath.assign(bfpath);
        siren_printf(SIREN_INFO, "use bf debug path %s", bfRecordingPath.c_str());

        std::string bfRawPath(config.debug_config.recording_path);
        bfRawPath.append("/bf_raw_debug.pcm");
        bfRawRecordingPath.assign(bfRawPath);
        siren_printf(SIREN_INFO, "use bf raw debug path %s", bfRawRecordingPath.c_str());

        std::string vadPath(config.debug_config.recording_path);
        vadPath.append("/vad_debug.pcm");
        vadRecordingPath.assign(vadPath);
        siren_printf(SIREN_INFO, "use vad debug path %s", vadRecordingPath.c_str());

        std::string opuPath(config.debug_config.recording_path);
        opuPath.append("/opu_debug.pcm");
        opuRecordingPath.assign(opuPath);
        siren_printf(SIREN_INFO, "use opu debug path %s", opuRecordingPath.c_str());
    }

    ~SirenProcessorImpl() {

    }

    SirenProcessorImpl(const SirenProcessorImpl &) = delete;
    SirenProcessorImpl& operator=(const SirenProcessorImpl &) = delete;
    int init();
    void destroy();

    void process(char* datain, int lenin, int aecflag, int awakeflag, int sleepflag, int asrflag, int hotwordflag);
    int getResult(r2ad_msg_block** msglst, int *msgnum);
    void setSLSteer(float ho, float ver);
    void getMsgs(r2ad_msg_block** &pMsgLst, int &iMsgNum);
    void reset();
    void syncVTWord(std::vector<siren_vt_word> &words);
    int getVTInfo(std::string &vt_word, int &start, int &end, float &vt_energy);

    float getLastFrameEnergy();
    float getLastFrameThreshold();

    class ProcessState {
    public:
        bool asr = false;
        bool vadStart = false;
        bool dataOutput = false;
        bool canceled = false;
        bool awke = false;
        bool firstFrm = true;

        int frmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS;
        int lastDuration = 0;
        int aecFlag = -1;
        int awakeFlag = -1;
        int sleepFlag = -1;
        int asrFlag = -1;
        int asrMsgCheckFlag = 0;

        std::string lastAwakeInfo;

        void updateAndDumpStatus(int lenin, int aecflag, int awakeflag, int sleepflag, int asrflag);
    };
    class TinyAllocator {
    public:
        int msgNumCurr;
        int msgNumTotal;
        r2ad_msg_block **msgLst;

        int colNoNew;
        float** dataNoNew;

        int getMsgs(r2ad_msg_block** &pMsgLst, int &iMsgNum);
        int dumpMsg(r2ad_msg_block *msg);
    };


private:
    void getErrorInfo(float **data_mul, std::vector<int> &errorMic);
    bool fixErrorMic(std::vector<int> &errorMic);
    void dumpMsg(r2ad_msg_block *msg);
    void addMsg(r2ad_msg msgid, int msgdatalen, const char *data);
    void addMsg(r2ad_msg msgid, const char *sl);
    void addMsg(r2ad_msg msgid, r2mem_cod *cod);
    void clearMsgLst();
    void resetASR();



    SirenConfig &config;
    ProcessorMicInfoAdapter micinfo;
    ProcessorUnitAdapter unit;
    ProcessState state;
    TinyAllocator allocator;
    float slinfo[3];

    bool bf_record;
    bool bf_raw_record;
    bool vad_record;
    bool opu_record;

    std::string bfRecordingPath;
    std::string bfRawRecordingPath;
    std::string vadRecordingPath;
    std::string opuRecordingPath;

    std::ofstream bfRecordingStream;
    std::ofstream bfRawRecordingStream;
    std::ofstream vadRecordingStream;
    std::ofstream opuRecordingStream;
};

}

#endif
