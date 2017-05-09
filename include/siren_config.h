#ifndef SIREN_CONFIG_H_
#define SIREN_CONFIG_H_

#include <stdint.h>
#include <vector>
#include <string>


namespace BlackSiren {

struct MicPos {
    std::vector<long double> pos;
};


//JSON KEY
#define IPC_CHANNEL "channel"
#define IPC_DBUS "dbus"
#define IPC_BINDER "binder"
#define IPC_SHARE_MEM "share_mem"

#define KEY_BASIC_CONFIG "basic_config"
#define KEY_ALG_CONFIG "alg_config"
#define KEY_DEBUG_CONFIG "debug_config"

#define KEY_MIC_NUM "mic_num"
#define KEY_MIC_CHANNEL_NUM "mic_channel_num"
#define KEY_MIC_SAMPLE_RATE "mic_sample_rate"
#define KEY_MIC_AUDIO_BYTE "mic_audio_byte"
#define KEY_MIC_FRAME_LENGTH "mic_frame_length"

#define KEY_SIREN_IPC "siren_ipc"
#define KEY_SIREN_CHANNEL_RMEM "siren_channel_rmem"
#define KEY_SIREN_CHANNEL_WMEM "siren_channel_wmem"

#define KEY_SIREN_INPUT_ERR_RETRY_NUM "siren_input_err_retry_num"
#define KEY_SIREN_INPUT_ERR_RETRY_TIMEOUT "siren_input_err_retry_timeout"

#define KEY_SIREN_MONITOR_UDP_PORT "siren_monitor_udp_port"

#define KEY_ALG_USE_LEGACY_CONFIG_FILE "alg_use_legacy_config_file"
#define KEY_ALG_LEGACY_CONFIG_FILE_PATH "alg_legacy_config_file_path"
#define KEY_ALG_LAN "alg_lan"
#define KEY_ALG_RS_MICS "alg_rs_mics"
#define KEY_ALG_AEC "alg_aec"
#define KEY_ALG_AEC_MICS "alg_aec_mics"
#define KEY_ALG_AEC_REF_MICS "alg_aec_ref_mics"
#define KEY_ALG_AEC_SHIELD "alg_aec_shield"
#define KEY_ALG_AEC_AFF_CPUS "alg_aec_aff_cpus"
#define KEY_ALG_AEC_MAT_AFF_CPUS "alg_aec_mat_aff_cpus"

#define KEY_ALG_RAW_STREAM_SL_DIRECTION "alg_raw_stream_sl_direction"
#define KEY_ALG_RAW_STREAM_BF "alg_raw_stream_bf"
#define KEY_ALG_RAW_STREAM_AGC "alg_raw_stream_agc"

#define KEY_ALG_VT_ENABLE "alg_vt_enable"
#define KEY_ALG_VAD_ENABLE "alg_vad_enable"

#define KEY_ALG_VAD_MICS "alg_vad_mics"
#define KEY_ALG_MIC_POS "alg_mic_pos"
#define KEY_ALG_SL_MICS "alg_sl_mics"
#define KEY_ALG_BF_MICS "alg_bf_mics"
#define KEY_ALG_OPUS_COMPRESS "alg_opus_compress"

#define KEY_ALG_VT_PHOMOD "alg_vt_phomod"
#define KEY_ALG_VT_DNNMOD "alg_vt_dnnmod"

#define KEY_ALG_RS_DELAY_ON_LEFT_RIGHT_CHANNEL "alg_rs_delay_on_left_right_channel"

#define KEY_RAW_STREAM_CHANNEL_NUM "raw_stream_channel_num"
#define KEY_RAW_STREAM_SAMPLE_RATE "raw_stream_sample_rate"
#define KEY_RAW_STREAM_BYTE "raw_stream_byte"

#define KEY_DEBUG_MIC_ARRAY_RECORD "debug_mic_array_record"
#define KEY_DEBUG_PRE_RESULT_RECORD "debug_pre_result_record"
#define KEY_DEBUG_PROC_RESULT_RECORD "debug_proc_result_record"
#define KEY_DEBUG_RS_RECORD "debug_rs_record"
#define KEY_DEBUG_AEC_RECORD "debug_aec_record"
#define KEY_DEBUG_RECORD_PATH "debug_record_path"

struct AlgConfig {
    std::string alg_legacy_dir;
    std::vector<int> alg_rs_mics;
    std::vector<int> alg_aec_mics;
    std::vector<int> alg_aec_ref_mics;
    std::vector<int> alg_aec_aff_cpus;
    std::vector<int> alg_aec_mat_aff_cpus;
    std::vector<int> alg_vad_mics;
    std::vector<MicPos> alg_mic_pos;
    std::vector<int> alg_sl_mics;
    std::vector<int> alg_bf_mics;
    std::string alg_vt_config_file_path;
    std::string alg_vt_phomod;
    std::string alg_vt_dnnmod;

    int alg_lan;
    
    float alg_aec_shield = 200.0f;
    float alg_raw_stream_sl_direction = 180.0f;
    
    bool alg_use_legacy_ssp_config_file = true;
    bool alg_aec = true;
    bool alg_rs_delay_on_left_right_channel;
    bool alg_raw_stream_bf = true;
    bool alg_raw_stream_agc = true;
    bool alg_vt_enable = true;
    bool alg_vad_enable = true;
    bool alg_opus_compress = false;
    bool alg_use_legacy_vt_config_file = true;
};

struct RawStreamConfig {
    int raw_stream_channel_num = 1;
    int raw_stream_sample_rate = 16000;
    int raw_stream_byte = 2;
};

struct DebugConfig {
    bool mic_array_record;
    bool preprocessed_result_record;
    bool processed_result_record;
    bool rs_record;
    bool aec_record;

    std::string recording_path;
};

struct SirenConfig {
    int mic_num = 8;
    int mic_channel_num = 8;
    int mic_sample_rate = 48000;
    int mic_audio_byte = 4;
    int mic_frame_length = 10;

    bool siren_use_share_mem = false;
    unsigned long siren_recording_socket_wmem = 4 * 1024 * 1024;
    unsigned long siren_recording_socket_rmem = 6 * 1024 * 1024;
    
    int siren_input_err_retry_num = 5;
    int siren_input_err_retry_timeout = 100;

    int udp_port;

    struct AlgConfig alg_config;
    struct RawStreamConfig raw_stream_config;
    struct DebugConfig debug_config;
};

typedef int32_t config_error_t;
enum {
    CONFIG_OK = 0,
    CONFIG_ERROR_OPEN_FILE,
    CONFIG_ERROR_PARSE_FAIL,
    CONFIG_ERROR_UNKNOWN,
};

enum {
    CONFIG_LAN_ZH = 0,
    CONFIG_LAN_EN,
};

class SirenConfigurationManager {
public:
    SirenConfigurationManager(const char *file) {
        if (file == nullptr) {
            validPath = false;
        } else {
            config_file_path = file;
            validPath = true;
        }    
    }
    ~SirenConfigurationManager() {}
    config_error_t parseConfigFile();
    void updateConfigFile(bool &);
    SirenConfig& getConfigFile() {
        return siren_config;
    }

    config_error_t loadConfigFromJSON(std::string &, SirenConfig &);

private:
    bool validPath;
    std::string config_file_path;
    SirenConfig siren_config;
};

}

#endif
