#ifndef SIREN_H_
#define SIREN_H_

#include <stdint.h>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t siren_status_t;
typedef int32_t siren_event_t;
typedef int32_t siren_state_t;
typedef int32_t siren_vt_t;

typedef unsigned long siren_t;


typedef int (*init_input_stream_t)(void *token);
typedef void (*release_input_stream_t)(void *token);
typedef int (*start_input_stream_t)(void *token);
typedef void (*stop_input_stream_t)(void *token);
typedef int (*read_input_stream_t)(void *token, char *buff, int len);
typedef void (*on_err_input_stream_t)(void *token);

#define VT_TYPE_AWAKE 1
#define VT_TYPE_SLEEP 2
#define VT_TYPE_HOTWORD 3
#define VT_TYPE_OTHER 4

typedef struct {
    float vt_block_avg_score;
    float vt_block_min_score;
    float vt_classify_shield;

    bool vt_left_sil_det;
    bool vt_right_sil_det;
    bool vt_remote_check_with_aec;
    bool vt_remote_check_without_aec;
    bool vt_local_classify_check;
    
    std::string nnet_path;
} siren_vt_alg_config;

typedef struct {
    int vt_type;
    std::string vt_word;
    std::string vt_phone;
    bool use_default_config;
    siren_vt_alg_config alg_config;
} siren_vt_word;

typedef struct {
    init_input_stream_t init_input;
    release_input_stream_t release_input;
    start_input_stream_t start_input;
    stop_input_stream_t stop_input;
    read_input_stream_t read_input;
    on_err_input_stream_t on_err_input;
} siren_input_if_t;

typedef struct {
    int start;
    int end;
    float energy;
} vt_event_t;

#define VOICE_MASK (0x1 << 0)
#define SL_MASK (0x1 << 1)
#define VT_MASK (0x1 << 2)

typedef struct {
    siren_event_t event;
    int length;
    int flag;
   
    double sl;
    double background_energy;
    double background_threshold;
    
    vt_event_t vt;
    void *buff;
} voice_event_t;

#define HAS_VOICE(flag) ((flag & VOICE_MASK) != 0)
#define HAS_SL(flag) ((flag & SL_MASK) != 0)
#define HAS_VT(flag) ((flag & VT_MASK) != 0)

typedef void (*on_voice_event_t)(void *token, voice_event_t *voice_event);
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
    SIREN_VT_OK = 0,
    SIREN_VT_DUP,
    SIREN_VT_ERROR,
    SIREN_VT_NO_EXIT
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
siren_t start_siren_process_stream(siren_t siren, siren_proc_callback_t *callback);
siren_t start_siren_raw_stream(siren_t siren, siren_raw_stream_callback_t *callback);

void stop_siren_process_stream(siren_t siren);
void stop_siren_raw_stream(siren_t siren);
void stop_siren_stream(siren_t siren);

void set_siren_state(siren_t siren, siren_state_t state, siren_state_changed_callback_t *callback);
void set_siren_steer(siren_t siren, float ho, float ver);

void destroy_siren(siren_t siren);

siren_vt_t add_vt_word(siren_t siren, siren_vt_word *word, bool use_default_config);
siren_vt_t remove_vt_word(siren_t siren, const char *word);

int get_vt_word(siren_t siren, siren_vt_word **words);


void start_siren_monitor(siren_t siren, siren_net_callback_t *callback);
siren_status_t broadcast_siren_event(siren_t siren, char *data, int len);


#ifdef __cplusplus
}
#endif

#endif
