#ifndef ISIREN_H_
#define ISIREN_H_

#include "common.h"
#include "siren.h"

namespace BlackSiren {

enum {
    SIREN_CALLBACK_ON_STATE_CHANGED = 0,
};

enum {
    SIREN_REQUEST_MSG_INIT = 0,
    SIREN_REQUEST_MSG_START_PROCESS_STREAM,
    SIREN_REQUEST_MSG_STOP_PROCESS_STREAM,
    SIREN_REQUEST_MSG_START_RAW_STREAM,
    SIREN_REQUEST_MSG_STOP_RAW_STREAM,
    SIREN_REQUEST_MSG_STOP_STREAM,
    SIREN_REQUEST_MSG_SET_STATE,
    SIREN_REQUEST_MSG_SET_STEER,
    SIREN_REQUEST_MSG_DATA_PROCESS,
    SIREN_REQUEST_MSG_SYNC_VT_WORD_LIST,
    SIREN_REQUEST_MSG_DESTROY_ON_INIT,
    SIREN_REQUEST_MSG_DESTROY,


    SIREN_RESPONSE_MSG_ON_INIT_OK,
    SIREN_RESPONSE_MSG_ON_INIT_FAILED,
    SIREN_RESPONSE_MSG_ON_STATE_CHANGED,
    SIREN_RESPONSE_MSG_ON_VOICE_EVENT,
    SIREN_RESPONSE_MSG_ON_RAW_VOICE,
    SIREN_RESPONSE_MSG_ON_CALLBACK,
    SIREN_RESPONSE_MSG_ON_DESTROY,
};


//interface for siren
class ISiren {
public:
    ISiren() = default;
    virtual ~ISiren() {}
    virtual siren_status_t init_siren(void *token, const char *path, 
            siren_input_if_t *input) = 0;
    virtual siren_status_t start_siren_process_stream(siren_proc_callback_t 
            *callback) = 0;
    virtual siren_status_t start_siren_raw_stream(siren_raw_stream_callback_t *callback) = 0;
    virtual void stop_siren_process_stream() = 0;
    virtual void stop_siren_raw_stream() = 0;
    virtual void stop_siren_stream() = 0;
    
    virtual void set_siren_state(siren_state_t state,
            siren_state_changed_callback_t *callback) = 0;
    virtual void set_siren_steer(float ho, float ver) = 0;
    virtual void destroy_siren() = 0;
    virtual bool get_thread_key() = 0;
};

}

#endif
