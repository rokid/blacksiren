#ifndef SIREN_CONFIG_H_
#define SIREN_CONFIG_H_

#include <stdint.h>
#include <vector>
#include <string>


namespace BlackSiren {

struct MicPos {
    float x;
    float y;
    float z;
};
    
    
struct AlgConfig {
    bool alg_use_legacy_ssp_config_file = true;
    std::string alg_legacy_dir;
    int alg_lan;
    std::vector<int> alg_rs_mics;
    bool alg_aec = true;
    std::vector<int> alg_aec_mics;
    std::vector<int> alg_aec_ref_mics;
    float alg_aec_shield = 200.0f;
    std::vector<int> alg_aec_aff_cpus;
    float alg_raw_stream_sl_direction = 180.0f;
    bool alg_raw_stream_bf = true;
    bool alg_raw_stream_agc = true;
    
    bool alg_vt_enable = true;
    bool alg_vad_enable = true;

    std::vector<int> alg_vad_mics;
    std::vector<MicPos> alg_mic_pos;
    std::vector<int> alg_sl_mics;
    std::vector<int> alg_bf_mics;
    bool alg_opus_compress = false;

    bool alg_use_legacy_vt_config_file = true;
    std::string alg_vt_config_file_path;
    std::string alg_vt_phomod;
    std::string alg_vt_dnnmod;


    int alg_rs_delay_on_right_channel;
    int alg_rs_delay_on_left_channel;
};

struct RawStreamConfig {
    int raw_stream_channel_num = 1;
    int raw_stream_sample_rate = 16000;
    int raw_stream_byte = 2;
};

struct SirenConfig {
    int mic_channel_num = 8;
    int mic_sample_rate = 48000;
    int mic_audio_byte = 4;
    int mic_frame_length = 10;

    bool siren_use_share_mem = false;
    std::string siren_share_mem_file_path;
    unsigned long siren_recording_socket_wmem = 4 * 1024 * 1024;
    unsigned long siren_recording_socket_rmem = 6 * 1024 * 1024;
    int siren_raw_voice_block_size = 2;
    int siren_raw_voice_block_num = 524288;
    int siren_processed_voice_block_size = 65536;
    int siren_processed_voice_block_num = 256;
    
    int siren_input_err_retry_num = 5;
    int siren_input_err_retry_timeout = 100;

    struct AlgConfig alg_config;
    struct RawStreamConfig raw_stream_config;
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
    SirenConfigurationManager(const char *file) : config_file_path(file){}
    ~SirenConfigurationManager() {}
    config_error_t parseConfigFile();
    SirenConfig& getConfigFile() {
        return siren_config;
    }
    
private:
    std::string config_file_path;
    SirenConfig siren_config;
};

}

#endif
