#ifndef SIREN_PROXY_H_

#define SIREN_PROXY_H_

#include <thread>
#include <functional>
#include <future>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iterator>
#include <fstream>

#include "siren_config.h"
#include "siren_channel.h"
#include "siren_net.h"
#include "isiren.h"
#include "sutils.h"
#include "lfqueue.h"
#include "siren_alg.h"

namespace BlackSiren {

class SirenProxy;
class RecordingThread {
public:
    RecordingThread(SirenProxy *siren);
    ~RecordingThread();
   
    bool init();
    bool start();
    void stop();
    void pause();

    int getReader() {
        close (sockets[0]);
        return sockets[1];
    }

    int getWriter() {
        close (sockets[1]);
        return sockets[0];
    }

    bool isRecordingStart() {
        return recordingStart;
    }

    void setThread(std::thread& thread) {
        this->thread = std::move(thread);
    }

    std::thread& getThread() {
        return thread;
    }

    void recordingFn();
private:
    std::mutex startMutex;
    std::mutex termMutex;
    std::condition_variable startCond;
    std::thread thread;

    SirenProxy* pSiren; 
    char *frameBuffer;
    bool recordingStart;
    bool recordingTerm;

    int currentRetry;
    int errorRetry;
    int retryTimeout;

    int frameSize;
    int sockets[2];

    bool doMicRecording;
    std::string micRecording;
    std::ofstream micRecordingStream;
};

class SirenProxy : public ISiren {
public:
    SirenProxy() :
        stateChangeCallback([this](void *token, int current){
            siren_printf(SIREN_INFO, "recv change callback with current state %d", current);     
        }),
        requestResponseLaunch(false),
        waitingInit(false),
        sirenBaseInitFailed(false),
        waitStateChange(false),
        requestThreadStop(false),
        allocated_from_thread(false),
        global_config(nullptr),
        input_callback(nullptr),
        realStreamStart(false),
        procStreamStart(false),
        recordStreamStart(false),
        recordingThread(nullptr),
        requestQueue(32, nullptr),
        udpRecvStart(false)
    {

    }
    virtual ~SirenProxy() {}

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
        return allocated_from_thread;
    }

    siren_vt_t add_vt_word(siren_vt_word *word, bool use_default_settings);
    siren_vt_t remove_vt_word(const char *word);

    int get_vt_word(siren_vt_word **words);

    void clearThread() {
        if (recordingThread != nullptr) {
            delete recordingThread;
        }
    }

    void finishRequestThread() {
        requestThreadStop.store(true, std::memory_order_release);    
    }    

    void requestThreadHandler();
    void responseThreadHandler();
    void monitorThreadHandler();

    void start_siren_monitor(siren_net_callback_t *callback);
    siren_status_t broadcast_siren_event(char *data, int len); 
private:
    std::function<void(void*, int)> stateChangeCallback; 
    void *token;
    int hasVTWord(const char  *word, std::vector<siren_vt_word>::iterator &);
    
    friend class RecordingThread;
    void launchRequestThread();
    void launchResponseThread();
    void launchMonitorThread();

    void waitingRequestResponseThread();
    void stopRequestThread(bool onInit);
    bool requestResponseLaunch;
    std::mutex launchMutex;
    std::condition_variable launchCond;

    bool waitingInit;
    bool sirenBaseInitFailed;
    std::mutex initMutex;
    std::condition_variable initCond;

    std::mutex requestCallbackMutex;
    std::condition_variable requestCond;
    std::vector<InterstedResponse> waitMessage;
   
    std::mutex stateChangeMutex;
    std::condition_variable stateChangeCond;
    bool waitStateChange;

    std::thread requestThread;
    std::atomic_bool requestThreadStop;

    std::thread responseThread;
    std::atomic_bool responseThreadStop;

    bool allocated_from_thread;
    SirenConfigurationManager *global_config;
    siren_input_if_t *input_callback;
    siren_proc_callback_t *proc_callback;
    siren_net_callback_t *net_callback;

    bool realStreamStart;
    bool procStreamStart;
    bool recordStreamStart;

    RecordingThread *recordingThread;
    SirenSocketChannel requestChannel;
    SirenSocketChannel responseChannel;

    LFQueue requestQueue;  
    int siren_pid;
    siren_state_t prevState; 

    //monitor
    std::atomic_bool udpRecvStart;
    std::promise<void> udpRecvOncePromise;
    std::thread monitorThread;
    SirenUDPAgent udpAgent;

    //vt
    std::vector<siren_vt_word> vt_words;
    siren_vt_word *stored_words = nullptr;
    SirenPhonemeGen phonemeGen;
};


}



#endif
