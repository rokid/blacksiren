#ifndef ISIREN_H_
#define ISIREN_H_



#include "common.h"
#include "siren.h"

namespace BlackSiren {

enum {
    SIREN_REQUEST_MSG_INIT = 0,
    SIREN_REQUEST_MSG_START_PROCESS_STREAM = 1,
    SIREN_REQUEST_MSG_STOP_PROCESS_STREAM = 2,
    SIREN_REQUEST_MSG_START_RAW_STREAM = 3,
    SIREN_REQUEST_MSG_STOP_RAW_STREAM = 4,
    SIREN_REQUEST_MSG_STOP_STREAM = 5,
    SIREN_REQUEST_MSG_SET_STATE = 6,
    SIREN_REQUEST_MSG_SET_STEER = 7,
    SIREN_REQUEST_MSG_REBUILD_VT_WORD_LIST = 8,
    SIREN_REQUEST_MSG_DESTROY = 9,

    SIREN_RESPONSE_MSG_ON_INIT = 10,
    SIREN_RESPONSE_MSG_ON_STATE_CHANGED = 11,
    SIREN_RESPONSE_MSG_ON_VOICE_EVENT = 12,
    SIREN_RESPONSE_MSG_ON_RAW_VOICE = 13
};


//interface for siren
class ISiren {
public:
    ISiren() = default;
    virtual ~ISiren() {}
    virtual siren_status_t init_siren(const char *path, 
            siren_input_if_t *input) = 0;
    virtual void start_siren_process_stream(siren_proc_callback_t 
            *callback) = 0;
    virtual void start_siren_raw_stream(siren_raw_stream_callback_t *callback) = 0;
    virtual void stop_siren_process_stream() = 0;
    virtual void stop_siren_raw_stream() = 0;
    virtual void stop_siren_stream() = 0;
    
    virtual void set_siren_state(siren_state_t state,
            siren_state_changed_callback_t *callback) = 0;
    virtual void set_siren_steer(float ho, float ver) = 0;
    virtual void destroy_siren() = 0;
    virtual siren_status_t rebuild_vt_word_list(const char **vt_word_list) = 0;
};

}

#endif
