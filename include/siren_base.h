#ifndef SIREN_BASE_H_
#define SIREN_BASE_H_


#include <thread>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <fstream>

#include "lfqueue.h"
#include "siren_channel.h"
#include "siren_config.h"
#include "sutils.h"
#include "siren.h"
#include "isiren.h"

namespace BlackSiren {


//will work as Recording thread after fork
class SirenBase : public ISiren {
public:
    SirenBase(SirenConfig &config, int socket, SirenSocketReader &requestReader, SirenSocketWriter &resultWriter);
    virtual ~SirenBase();

    virtual siren_status_t init_siren(void *token, const char *path, siren_input_if_t *input) override;
    virtual siren_status_t start_siren_process_stream(siren_proc_callback_t *callback) override;
    virtual siren_status_t start_siren_raw_stream(siren_raw_stream_callback_t *callback) override;
    virtual void stop_siren_process_stream() override;
    virtual void stop_siren_raw_stream() override;
    virtual void stop_siren_stream() override;

    virtual void set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) override;
    virtual void set_siren_steer(float ho, float var) override;
    virtual void destroy_siren() override;
    
    virtual bool get_thread_key() override {
        return false;
    }

    void responseThreadHandler();
    void processThreadHandler();
    void main();
private:
    std::function<void(int)> onStateChanged;
    std::thread responseThread;
    void launchResponseThread();
    
    void responseInitDone();
    void sync_vt_word(Message *copiedMessage);

    std::thread processThread;
    void launchProcessThread();
    void waitingProcessInit();
    void loopRecording();
        
    bool processInitFailed;


    std::mutex initMutex;
    std::condition_variable initCond;
    bool processThreadInit;
    

    SirenConfig& config;
    SirenSocketReader& requestReader;
    SirenSocketWriter& resultWriter;

    int frameSize;
    char *frameBuffer;
    int socket;

    std::atomic_bool recordingExit;

    std::mutex recordingMutex;
    std::condition_variable recordingCond;
    bool recordingStart;

    LFQueue processQueue;
    LFQueue recordingQueue;

    bool doPreRecording;
    std::string preRecording;
    std::ofstream preRecordingStream;

    bool doProcRecording;
    std::string procRecording;
    std::ofstream procRecordingStream;


};


}
#endif
