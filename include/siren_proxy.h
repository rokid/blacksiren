#ifndef SIREN_PROXY_H_
#define SIREN_PROXY_H_

#include <thread>
#include <unistd.h>
#include <mutex>
#include <condition_variable>

#include "siren_config.h"
#include "siren_channel.h"
#include "isiren.h"
#include "sutils.h"

namespace BlackSiren {

class SirenProxy;
class ProxyRequestThread {
public:
    ProxyRequestThread(SirenProxy *siren):
            pSiren(siren) {}
    ~ProxyRequestThread() {
    
    }

private:
    SirenProxy* pSiren;
};


class ProxyResponseThread {
public:
    ProxyResponseThread(SirenProxy *siren):
            pSiren(siren) {}
    ~ProxyResponseThread() {
    
    }

private:
    SirenProxy* pSiren;
};

class RecordingThread {
public:
    RecordingThread(SirenProxy *siren);
    ~RecordingThread();
   
    bool init();
    void start();
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

};

class SirenProxy : public ISiren {
public:
    SirenProxy() : allocated_from_thread(false),
        global_config(nullptr),
        input_callback(nullptr),
        realStreamStart(false),
        procStreamStart(false),
        recordStreamStart(false),
        requestThread(nullptr),
        responseThread(nullptr),
        recordingThread(nullptr)
    {

    }
    virtual ~SirenProxy() = default;

    virtual siren_status_t init_siren(const char *path, siren_input_if_t *input) override;
    virtual void start_siren_process_stream(siren_proc_callback_t *callback) override;
    virtual void start_siren_raw_stream(siren_raw_stream_callback_t *callback) override;
    virtual void stop_siren_process_stream() override;
    virtual void stop_siren_raw_stream() override;
    virtual void stop_siren_stream() override;

    virtual void set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) override;
    virtual void set_siren_steer(float ho, float var) override;
    virtual void destroy_siren() override;
    virtual siren_status_t rebuild_vt_word_list(const char **vt_word_list, int num) override;

    virtual bool get_thread_key() override {
        return allocated_from_thread;
    }

    void clearThread() {
        if (requestThread != nullptr) {
            delete requestThread;
        }

        if (responseThread != nullptr) {
            delete responseThread;
        }

        if (recordingThread != nullptr) {
            delete recordingThread;
        }
    }

private:
    friend class ProxyRequestThread;
    friend class ProxyResponseThread;
    friend class RecordingThread;

    
    bool allocated_from_thread;
    SirenConfigurationManager *global_config;
    siren_input_if_t *input_callback;

    bool realStreamStart;
    bool procStreamStart;
    bool recordStreamStart;

    ProxyRequestThread *requestThread;
    ProxyResponseThread *responseThread;
    RecordingThread *recordingThread;

    int siren_pid;
};


}



#endif
