#ifndef SIREN_PROXY_H_
#define SIREN_PROXY_H_

#include <thread>

#include "isiren.h"
#include "sutils.h"

namespace BlackSiren {
 
class SirenProxy;
class ProxyRequestThread {
    public:


};


class ProxyResponseThread {
    public:

};


class SirenProxy : public ISiren {
public:
    SirenProxy() : allocated_from_thread(false) {}
    virtual ~SirenProxy();

    virtual siren_status_t init_siren(const char *path, siren_input_if_t *input) override;
    virtual void start_siren_process_stream(siren_proc_callback_t *callback) override;
    virtual void start_siren_raw_stream(siren_raw_stream_callback_t *callback) override;
    virtual void stop_siren_process_stream() override;
    virtual void stop_siren_raw_stream() override;
    virtual void stop_siren_stream() override;

    virtual void set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) override;
    virtual void set_siren_steer(float ho, float var) override;
    virtual void destroy_siren() override;
    virtual siren_status_t rebuild_vt_word_list(const char **vt_word_list) override;

    friend class ProxyRequestThread;
    friend class ProxyResponseThread;

    bool getThreadKey() {
        return allocated_from_thread;
    }

private:
    bool allocated_from_thread;    
    
    
};

    
}



#endif
