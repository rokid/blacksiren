#ifndef SIREN_H_
#define SIREN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t siren_status_t;
typedef int32_t siren_event_t;
typedef int32_t siren_state_t;

typedef unsigned long siren_t;

typedef int (*init_input_stream_t)(void *token);
typedef void (*release_input_stream_t)(void *token);
typedef void (*start_input_stream_t)(void *token);
typedef void (*stop_input_stream_t)(void *token);
typedef int (*read_input_stream_t)(void *token, char *buff, int len);
typedef void (*on_err_input_stream_t)(void *token);


typedef struct {
    init_input_stream_t init_input;
    release_input_stream_t release_input;
    start_input_stream_t start_input;
    stop_input_stream_t stop_input;
    read_input_stream_t read_input;
    on_err_input_stream_t on_err_input;
} siren_input_if_t;

typedef void (*on_voice_event_t)(void *token, int length, siren_event_t event,
                                 void *buff, int has_sl,
                                 int has_voice, double sl_degree, double energy, double threshold,
                                 int has_voiceprint);

typedef void (*on_net_event_t)(void *token, char *data, int len);

typedef struct {
    on_voice_event_t voice_event_callback;
} siren_proc_callback_t;

typedef struct {
    on_net_event_t net_event_callback;
} siren_net_callback_t;

typedef void (*on_raw_voice_t)(void *token, int length, void *buff);

typedef struct {
    on_raw_voice_t raw_voice_callback;
} siren_raw_stream_callback_t;

typedef void (*on_siren_state_changed_t)(void *token, int current);

typedef struct {
    on_siren_state_changed_t state_changed_callback;
} siren_state_changed_callback_t;

enum {
    SIREN_STATUS_OK = 0,
    SIREN_STATUS_CONFIG_ERROR,
    SIREN_STATUS_CONFIG_NO_FOUND,
    SIREN_STATUS_ERROR
};

enum {
    SIREN_EVENT_VAD_START = 100,
    SIREN_EVENT_VAD_DATA,
    SIREN_EVENT_VAD_END,
    SIREN_EVENT_VAD_CANCEL,
    SIREN_EVENT_WAKE_VAD_START,
    SIREN_EVENT_WAKE_VAD_DATA,
    SIREN_EVENT_WAKE_VAD_END,
    SIREN_EVENT_WAKE_PRE,
    SIREN_EVENT_WAKE_NOCMD,
    SIREN_EVENT_WAKE_CMD,
    SIREN_EVENT_WAKE_CANCEL,
    SIREN_EVENT_SLEEP,
    SIREN_EVENT_HOTWORD,
    SIREN_EVENT_SR,
    SIREN_EVENT_VOICE_PRINT,
    SIREN_EVENT_DIRTY
};

enum {
    SIREN_STATE_AWAKE =1 ,
    SIREN_STATE_SLEEP
};

siren_t init_siren(void *token, const char *path, siren_input_if_t *input);
void start_siren_process_stream(siren_t siren, siren_proc_callback_t *callback);
void start_siren_raw_stream(siren_t siren, siren_raw_stream_callback_t *callback);
void stop_siren_process_stream(siren_t siren);
void stop_siren_raw_stream(siren_t siren);
void stop_siren_stream(siren_t siren);
void set_siren_state(siren_t siren, siren_state_t state, siren_state_changed_callback_t *callback);
void set_siren_steer(siren_t siren, float ho, float ver);
void destroy_siren(siren_t siren);
siren_status_t rebuild_vt_word_list(siren_t siren, const char **vt_word_list);

void start_siren_monitor(siren_t siren, siren_net_callback_t *callback);
void broadcast_siren_event(siren_t siren, char *data, int len);


#ifdef __cplusplus
}
#endif

#endif
