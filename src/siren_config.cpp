
#include <stdint.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <curl/curl.h>
#include <netdb.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sutils.h"
#include "siren_config.h"
#include "json.h"

#ifdef CONFIG_LEGACY_SIREN_TEST
#define LEGACY_ALG_DIR_EN "/system/workdir_en"
#define LEGACY_ALG_DIR_CN "/system/workdir_cn"

#endif

namespace BlackSiren {

static std::string getAddressByHostname(const char *hostname) {
    std::string t;
    if (hostname == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "host name is null");
        return t;
    }

    hostent *record = gethostbyname(hostname);
    if (record == NULL) {
        siren_printf(BlackSiren::SIREN_ERROR, "cannot find hostname %s", hostname);
        return t;
    }

    in_addr *addr = (in_addr *)record->h_addr;
    t = inet_ntoa(* addr);

    return t;
}



template <typename T>
std::string build_printable_indx(const std::vector<T> &from) {
    std::string result;
    for (T j : from) {
        result.append(std::to_string(j)).append(" ");
    }
    return result;
}


config_error_t SirenConfigurationManager::loadConfigFromJSON(std::string &contents, SirenConfig &siren_config) {
    json_object *config_object = json_tokener_parse(contents.c_str());
    json_object *basic_config = nullptr;
    json_object *alg_config = nullptr;

    //basic conifg object
    json_object *mic_channel_num_object = nullptr;
    json_object *mic_sample_rate_object = nullptr;
    json_object *mic_audio_byte_object = nullptr;
    json_object *mic_frame_length_object = nullptr;
    json_object *siren_ipc_object = nullptr;
    json_object *siren_channel_rmem_object = nullptr;
    json_object *siren_channel_wmem_object = nullptr;
    json_object *siren_input_err_retry_num_object = nullptr;
    json_object *siren_input_err_retry_timeout_object = nullptr;

    if (config_object == nullptr) {
        siren_printf(SIREN_ERROR, "parse json failed");
        return CONFIG_ERROR_PARSE_FAIL;
    }

    //get basic config
    if (TRUE != json_object_object_get_ex(config_object, KEY_BASIC_CONFIG, &basic_config)) {
        siren_printf(SIREN_ERROR, "cannot find basic config");
        json_object_put(config_object);
        return CONFIG_ERROR_PARSE_FAIL;
    } 
    
    if (TRUE != json_object_object_get_ex(config_object, KEY_ALG_CONFIG, &alg_config)) {
        siren_printf(SIREN_ERROR, "cannot find alg config");
        json_object_put(basic_config);
        json_object_put(config_object);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    json_type type;
    if (TRUE == json_object_object_get_ex(basic_config, KEY_MIC_CHANNEL_NUM, &mic_channel_num_object)) {
        //KEY_MIC_CHANNEL_NUM
        if ((type = json_object_get_type(mic_channel_num_object)) == json_type_int) {
            siren_config.mic_channel_num = json_object_get_int(mic_channel_num_object);
            siren_printf(SIREN_INFO, "set mic channel num to %d", siren_config.mic_channel_num);
            json_object_put(mic_channel_num_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_MIC_CHANNEL_NUM);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(basic_config, KEY_MIC_SAMPLE_RATE, &mic_sample_rate_object)) {
        if ((type = json_object_get_type(mic_sample_rate_object)) == json_type_int) {
            siren_config.mic_sample_rate = json_object_get_int(mic_sample_rate_object);
            siren_printf(SIREN_INFO, "set mic sample rate to %d", siren_config.mic_sample_rate);
            json_object_put(mic_sample_rate_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_MIC_SAMPLE_RATE);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(basic_config, KEY_MIC_AUDIO_BYTE, &mic_audio_byte_object)) {
        if ((type = json_object_get_type(mic_audio_byte_object)) == json_type_int) {
            siren_config.mic_audio_byte = json_object_get_int(mic_audio_byte_object);
            siren_printf(SIREN_INFO, "set mic audio byte to %d", siren_config.mic_audio_byte);
            json_object_put(mic_audio_byte_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_MIC_AUDIO_BYTE);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(basic_config, KEY_MIC_FRAME_LENGTH, &mic_frame_length_object)) {
        if ((type = json_object_get_type(mic_frame_length_object)) == json_type_int) {
            siren_config.mic_frame_length = json_object_get_int(mic_frame_length_object);
            siren_printf(SIREN_INFO, "set mic frame length to %d", siren_config.mic_frame_length);
            json_object_put(mic_frame_length_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_MIC_FRAME_LENGTH);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(basic_config, KEY_SIREN_IPC, &siren_ipc_object)) {
        if ((type = json_object_get_type(siren_ipc_object)) == json_type_string) {
            const char *ipc_type = json_object_get_string(siren_ipc_object);
            siren_printf(SIREN_INFO, "use ipc %s", ipc_type);
            if (!strcmp(ipc_type, IPC_CHANNEL)) {
                siren_config.siren_use_share_mem = false;
            } else {
                siren_config.siren_use_share_mem = true;
            }
            json_object_put(siren_ipc_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_SIREN_IPC);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(basic_config, KEY_SIREN_CHANNEL_RMEM, &siren_channel_rmem_object)) {
        if ((type = json_object_get_type(siren_channel_rmem_object)) == json_type_int) {
            siren_config.siren_recording_socket_rmem = static_cast<unsigned long>(json_object_get_int64(siren_channel_rmem_object));
            siren_printf(SIREN_INFO, "set channel rmem to %ld", siren_config.siren_recording_socket_rmem);
            json_object_put(siren_channel_rmem_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_SIREN_CHANNEL_RMEM);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(basic_config, KEY_SIREN_CHANNEL_WMEM, &siren_channel_wmem_object)) {
        if ((type = json_object_get_type(siren_channel_wmem_object)) == json_type_int) {
            siren_config.siren_recording_socket_wmem = static_cast<unsigned long>(json_object_get_int64(siren_channel_wmem_object));
            siren_printf(SIREN_INFO, "set channel wmem to %ld", siren_config.siren_recording_socket_wmem);
            json_object_put(siren_channel_wmem_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_SIREN_CHANNEL_WMEM);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(basic_config, KEY_SIREN_INPUT_ERR_RETRY_NUM, &siren_input_err_retry_num_object)) {
        if ((type = json_object_get_type(siren_input_err_retry_num_object)) == json_type_int) {
            siren_config.siren_input_err_retry_num = json_object_get_int(siren_input_err_retry_num_object);
            siren_printf(SIREN_INFO, "set input retry num to %d", siren_config.siren_input_err_retry_num);
            json_object_put(siren_input_err_retry_num_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_SIREN_INPUT_ERR_RETRY_NUM);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(basic_config, KEY_SIREN_INPUT_ERR_RETRY_TIMEOUT, &siren_input_err_retry_timeout_object)) {
        if ((type = json_object_get_type(siren_input_err_retry_timeout_object)) == json_type_int) {
            siren_config.siren_input_err_retry_timeout = json_object_get_int(siren_input_err_retry_timeout_object);
            siren_printf(SIREN_INFO, "set input retry timeout to %d", siren_config.siren_input_err_retry_timeout);
            json_object_put(siren_input_err_retry_timeout_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_SIREN_INPUT_ERR_RETRY_TIMEOUT);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    json_object *alg_use_legacy_config_file_object = nullptr;
    json_object *alg_legacy_config_file_path_object = nullptr;
    json_object *alg_lan_object = nullptr;
    json_object *alg_rs_mics_object = nullptr;
    json_object *alg_aec_object = nullptr;
    json_object *alg_aec_mics_object = nullptr;
    json_object *alg_aec_ref_mics_object = nullptr;
    json_object *alg_aec_shield_object = nullptr;
    json_object *alg_aec_aff_cpus_object = nullptr;
    json_object *alg_aec_mat_aff_cpus_object = nullptr;

    json_object *alg_raw_stream_sl_direction_object = nullptr;
    json_object *alg_raw_stream_bf_object = nullptr;
    json_object *alg_raw_stream_agc_object = nullptr;

    json_object *alg_vt_enable_object = nullptr;
    json_object *alg_vad_enable_object = nullptr;

    json_object *alg_vad_mics_object = nullptr;
    json_object *alg_mic_pos_object = nullptr;
    json_object *alg_sl_mics_object = nullptr;
    json_object *alg_bf_mics_object = nullptr;
    json_object *alg_opus_compress_object = nullptr;
    json_object *alg_vt_phomod_object = nullptr;
    json_object *alg_vt_dnnmod_object = nullptr;

    json_object *alg_rs_delay_on_left_right_channel_object = nullptr;
    json_object *alg_raw_stream_channel_num_object = nullptr;
    json_object *alg_raw_stream_sample_rate_object = nullptr;
    json_object *alg_raw_stream_byte_object = nullptr;

    //handle alg confit
    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_USE_LEGACY_CONFIG_FILE, &alg_use_legacy_config_file_object)) {
        if ((type = json_object_get_type(alg_use_legacy_config_file_object)) == json_type_boolean) {
            bool use_legacy_config = json_object_get_boolean(alg_use_legacy_config_file_object);
            siren_printf(SIREN_INFO, "set input retry timeout to %d", use_legacy_config);
            if (use_legacy_config) {
                siren_config.alg_config.alg_use_legacy_ssp_config_file = true;
                siren_config.alg_config.alg_use_legacy_vt_config_file = true;
            } else {
                siren_config.alg_config.alg_use_legacy_ssp_config_file = false;
                siren_config.alg_config.alg_use_legacy_vt_config_file = false;
            }

            json_object_put(alg_use_legacy_config_file_object);
        }
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_USE_LEGACY_CONFIG_FILE);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_LEGACY_CONFIG_FILE_PATH, &alg_legacy_config_file_path_object)) {
        if ((type = json_object_get_type(alg_legacy_config_file_path_object)) == json_type_string) {
            const char *legacy_file_path = json_object_get_string(alg_legacy_config_file_path_object);
            siren_printf(SIREN_INFO, "legacy file path set to %s", legacy_file_path);
            siren_config.alg_config.alg_legacy_dir = legacy_file_path;
        }
        json_object_put(alg_legacy_config_file_path_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_LEGACY_CONFIG_FILE_PATH);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_LAN, &alg_lan_object)) {
        if ((type = json_object_get_type(alg_lan_object)) == json_type_string) {
            const char *lan = json_object_get_string(alg_lan_object);
            if (lan != nullptr) {
                if (!strcmp(lan, "zh")) {
                    siren_config.alg_config.alg_lan = 0;
                } else if (!strcmp(lan, "en")) {
                    siren_config.alg_config.alg_lan = 1;
                } else {
                    siren_config.alg_config.alg_lan = 0;
                }
            }
        }
        json_object_put(alg_lan_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_LAN);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_RS_MICS, &alg_rs_mics_object)) {
        if ((type = json_object_get_type(alg_rs_mics_object)) == json_type_array) {
            int len = json_object_array_length(alg_rs_mics_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *json_idx = json_object_array_get_idx(alg_rs_mics_object, i);
                    if ((type = json_object_get_type(json_idx)) == json_type_int) {
                        int rs_idx = json_object_get_int(json_idx);
                        siren_config.alg_config.alg_rs_mics.push_back(rs_idx);
                    }
                    json_object_put(json_idx);
                }

                siren_printf(SIREN_INFO, "rs mics: %s", build_printable_indx(siren_config.alg_config.alg_rs_mics).c_str());
            }
        }

        json_object_put(alg_rs_mics_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_RS_MICS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_AEC, &alg_aec_object)) {
        if ((type = json_object_get_type(alg_aec_object)) == json_type_boolean) {
            siren_config.alg_config.alg_aec = json_object_get_boolean(alg_aec_object);
            siren_printf(SIREN_INFO, "enable aec %d", siren_config.alg_config.alg_aec);
        }

        json_object_put(alg_aec_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_AEC);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_AEC_MICS, &alg_aec_mics_object)) {
        if ((type = json_object_get_type(alg_aec_mics_object)) == json_type_array) {
            int len = json_object_array_length(alg_aec_mics_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *json_idx = json_object_array_get_idx(alg_aec_mics_object, i);
                    if ((type = json_object_get_type(json_idx)) == json_type_int) {
                        int aec_idx = json_object_get_int(json_idx);
                        siren_config.alg_config.alg_aec_mics.push_back(aec_idx);
                    }
                    json_object_put(json_idx);
                }
            }

            siren_printf(SIREN_INFO, "aec mics: %s",
                         build_printable_indx(siren_config.alg_config.alg_aec_mics).c_str());
        }
        json_object_put(alg_aec_mics_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_AEC_MICS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_AEC_REF_MICS, &alg_aec_ref_mics_object)) {
        if ((type = json_object_get_type(alg_aec_ref_mics_object)) == json_type_array) {
            int len = json_object_array_length(alg_aec_ref_mics_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *json_idx = json_object_array_get_idx(alg_aec_ref_mics_object, i);
                    if ((type = json_object_get_type(json_idx)) == json_type_int) {
                        int aec_ref_idx = json_object_get_int(json_idx);
                        siren_config.alg_config.alg_aec_ref_mics.push_back(aec_ref_idx);
                    }
                    json_object_put(json_idx);
                }
            }

            siren_printf(SIREN_INFO, "aec ref mics: %s",
                         build_printable_indx(siren_config.alg_config.alg_aec_ref_mics).c_str());
        }
        json_object_put(alg_aec_mics_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_AEC_REF_MICS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_AEC_SHIELD, &alg_aec_shield_object)) {
        if ((type = json_object_get_type(alg_aec_shield_object)) == json_type_double) {
            siren_config.alg_config.alg_aec_shield = static_cast<float>(json_object_get_double(alg_aec_shield_object));
            siren_printf(SIREN_INFO, "set aec shield to %f", siren_config.alg_config.alg_aec_shield);
        }
        json_object_put(alg_aec_shield_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_AEC_SHIELD);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_AEC_AFF_CPUS, &alg_aec_aff_cpus_object)) {
        if ((type = json_object_get_type(alg_aec_aff_cpus_object)) == json_type_array) {
            int len = json_object_array_length(alg_aec_aff_cpus_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *json_idx = json_object_array_get_idx(alg_aec_aff_cpus_object, i);
                    if ((type = json_object_get_type(json_idx)) == json_type_int) {
                        int cpu_aff_idx = json_object_get_int(json_idx);
                        siren_config.alg_config.alg_aec_aff_cpus.push_back(cpu_aff_idx);
                    }
                    json_object_put(json_idx);
                }
            }

            siren_printf(SIREN_INFO, "aec aff cpus: %s",
                         build_printable_indx(siren_config.alg_config.alg_aec_aff_cpus).c_str());
        }
        json_object_put(alg_aec_aff_cpus_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_AEC_AFF_CPUS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_AEC_MAT_AFF_CPUS, &alg_aec_mat_aff_cpus_object)) {
        if ((type = json_object_get_type(alg_aec_mat_aff_cpus_object)) == json_type_array) {
            int len = json_object_array_length(alg_aec_mat_aff_cpus_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *json_idx = json_object_array_get_idx(alg_aec_mat_aff_cpus_object, i);
                    if ((type = json_object_get_type(json_idx)) == json_type_int) {
                        int cpu_mat_aff_idx = json_object_get_int(json_idx);
                        siren_config.alg_config.alg_aec_mat_aff_cpus.push_back(cpu_mat_aff_idx);
                    }
                    json_object_put(json_idx);
                }
            }

            siren_printf(SIREN_INFO, "aec mat aff cpus: %s",
                         build_printable_indx(siren_config.alg_config.alg_aec_mat_aff_cpus).c_str());
        }
        json_object_put(alg_aec_mat_aff_cpus_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_AEC_MAT_AFF_CPUS);
        return CONFIG_ERROR_PARSE_FAIL;
    }


    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_RAW_STREAM_SL_DIRECTION, &alg_raw_stream_sl_direction_object)) {
        if ((type = json_object_get_type(alg_raw_stream_sl_direction_object)) == json_type_double) {
            siren_config.alg_config.alg_raw_stream_sl_direction = static_cast<float>(json_object_get_double(alg_raw_stream_sl_direction_object));
            siren_printf(SIREN_INFO, "set raw stream sl direction to %f", siren_config.alg_config.alg_raw_stream_sl_direction);
        }
        json_object_put(alg_raw_stream_sl_direction_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_RAW_STREAM_SL_DIRECTION);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_RAW_STREAM_BF, &alg_raw_stream_bf_object)) {
        if ((type = json_object_get_type(alg_raw_stream_bf_object)) == json_type_boolean) {
            siren_config.alg_config.alg_raw_stream_bf = json_object_get_boolean(alg_raw_stream_bf_object);
            siren_printf(SIREN_INFO, "enable raw stream bf %d", siren_config.alg_config.alg_raw_stream_bf);
        }
        json_object_put(alg_raw_stream_bf_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_RAW_STREAM_BF);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_RAW_STREAM_AGC, &alg_raw_stream_agc_object)) {
        if ((type = json_object_get_type(alg_raw_stream_agc_object)) == json_type_boolean) {
            siren_config.alg_config.alg_raw_stream_agc = json_object_get_boolean(alg_raw_stream_agc_object);
            siren_printf(SIREN_INFO, "enable raw stream agc %d", siren_config.alg_config.alg_raw_stream_bf);
        }
        json_object_put(alg_raw_stream_agc_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_RAW_STREAM_AGC);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_VT_ENABLE, &alg_vt_enable_object)) {
        if ((type = json_object_get_type(alg_vt_enable_object)) == json_type_boolean) {
            siren_config.alg_config.alg_vt_enable = json_object_get_boolean(alg_vt_enable_object);
            siren_printf(SIREN_INFO, "enable vt %d", siren_config.alg_config.alg_vt_enable);
        }
        json_object_put(alg_vt_enable_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_VT_ENABLE);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_VAD_ENABLE, &alg_vad_enable_object)) {
        if ((type = json_object_get_type(alg_vad_enable_object)) == json_type_boolean) {
            siren_config.alg_config.alg_vad_enable = json_object_get_boolean(alg_vad_enable_object);
            siren_printf(SIREN_INFO, "enable vad %d", siren_config.alg_config.alg_vad_enable);
        }
        json_object_put(alg_vad_enable_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_VAD_ENABLE);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_VAD_MICS, &alg_vad_mics_object)) {
        if ((type = json_object_get_type(alg_vad_mics_object)) == json_type_array) {
            int len = json_object_array_length(alg_vad_mics_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *json_idx = json_object_array_get_idx(alg_vad_mics_object, i);
                    if ((type = json_object_get_type(json_idx)) == json_type_int) {
                        int vad_mic_idx = json_object_get_int(json_idx);
                        siren_config.alg_config.alg_vad_mics.push_back(vad_mic_idx);
                    }
                    json_object_put(json_idx);
                }
            }

            siren_printf(SIREN_INFO, "vad mics: %s",
                         build_printable_indx(siren_config.alg_config.alg_vad_mics).c_str());
        }
        json_object_put(alg_vad_mics_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_VAD_MICS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_MIC_POS, &alg_mic_pos_object)) {
        if ((type = json_object_get_type(alg_mic_pos_object)) == json_type_array) {
            int len = json_object_array_length(alg_mic_pos_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *pos_idx = json_object_array_get_idx(alg_mic_pos_object, i);
                    if ((type = json_object_get_type(pos_idx)) == json_type_array) {
                        int mic_pos_len = json_object_array_length(pos_idx);
                        MicPos mic;
                        for (int j = 0; j < mic_pos_len; j++) {
                            json_object *pos_i = json_object_array_get_idx(pos_idx, j);
                            if ((type = json_object_get_type(pos_i)) == json_type_double) {
                                mic.pos.push_back(json_object_get_double(pos_i));
                            }
                        }
                        siren_config.alg_config.alg_mic_pos.push_back(mic);
                    }
                }
            }

            for (MicPos mic : siren_config.alg_config.alg_mic_pos) {
                siren_printf(SIREN_INFO, "mic pos: %s",
                             build_printable_indx<long double>(mic.pos).c_str());
            }
        }
        json_object_put(alg_mic_pos_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_MIC_POS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_SL_MICS, &alg_sl_mics_object)) {
        if ((type = json_object_get_type(alg_sl_mics_object)) == json_type_array) {
            int len = json_object_array_length(alg_sl_mics_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *json_idx = json_object_array_get_idx(alg_sl_mics_object, i);
                    if ((type = json_object_get_type(json_idx)) == json_type_int) {
                        int sl_mic_idx = json_object_get_int(json_idx);
                        siren_config.alg_config.alg_sl_mics.push_back(sl_mic_idx);
                    }
                    json_object_put(json_idx);
                }
            }

            siren_printf(SIREN_INFO, "sl mics: %s",
                         build_printable_indx(siren_config.alg_config.alg_sl_mics).c_str());
        }
        json_object_put(alg_sl_mics_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_SL_MICS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_BF_MICS, &alg_bf_mics_object)) {
        if ((type = json_object_get_type(alg_bf_mics_object)) == json_type_array) {
            int len = json_object_array_length(alg_bf_mics_object);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    json_object *json_idx = json_object_array_get_idx(alg_bf_mics_object, i);
                    if ((type = json_object_get_type(json_idx)) == json_type_int) {
                        int bf_mic_idx = json_object_get_int(json_idx);
                        siren_config.alg_config.alg_bf_mics.push_back(bf_mic_idx);
                    }
                    json_object_put(json_idx);
                }
            }

            siren_printf(SIREN_INFO, "bf mics: %s",
                         build_printable_indx(siren_config.alg_config.alg_bf_mics).c_str());
        }
        json_object_put(alg_vad_mics_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_BF_MICS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_OPUS_COMPRESS, &alg_opus_compress_object)) {
        if ((type = json_object_get_type(alg_opus_compress_object)) == json_type_boolean) {
            siren_config.alg_config.alg_opus_compress = json_object_get_boolean(alg_opus_compress_object);
            siren_printf(SIREN_INFO, "enable opus compression %d", siren_config.alg_config.alg_opus_compress);
        }
        json_object_put(alg_opus_compress_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_OPUS_COMPRESS);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_VT_PHOMOD, &alg_vt_phomod_object)) {
        if ((type = json_object_get_type(alg_vt_phomod_object)) == json_type_string) {
            const char *phomod = json_object_get_string(alg_vt_phomod_object);
            siren_printf(SIREN_INFO, "phomod file path set to %s", phomod);
            siren_config.alg_config.alg_vt_phomod = phomod;
        }
        json_object_put(alg_vt_phomod_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_VT_PHOMOD);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_VT_DNNMOD, &alg_vt_dnnmod_object)) {
        if ((type = json_object_get_type(alg_vt_dnnmod_object)) == json_type_string) {
            const char *dnnmod = json_object_get_string(alg_vt_dnnmod_object);
            siren_printf(SIREN_INFO, "dnnmod file path set to %s", dnnmod);
            siren_config.alg_config.alg_vt_dnnmod = dnnmod;
        }
        json_object_put(alg_vt_dnnmod_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_VT_DNNMOD);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_ALG_RS_DELAY_ON_LEFT_RIGHT_CHANNEL, &alg_rs_delay_on_left_right_channel_object)) {
        if ((type = json_object_get_type(alg_rs_delay_on_left_right_channel_object)) == json_type_boolean) {
            siren_config.alg_config.alg_rs_delay_on_left_right_channel = json_object_get_boolean(alg_rs_delay_on_left_right_channel_object);
            siren_printf(SIREN_INFO, "enable rs delay %d", siren_config.alg_config.alg_rs_delay_on_left_right_channel);
        }
        json_object_put(alg_rs_delay_on_left_right_channel_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_ALG_RS_DELAY_ON_LEFT_RIGHT_CHANNEL);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_RAW_STREAM_CHANNEL_NUM, &alg_raw_stream_channel_num_object)) {
        if ((type = json_object_get_type(alg_raw_stream_channel_num_object)) == json_type_int) {
            siren_config.raw_stream_config.raw_stream_channel_num = json_object_get_int(alg_raw_stream_channel_num_object);
            siren_printf(SIREN_INFO, "raw stream channel num %d", siren_config.raw_stream_config.raw_stream_channel_num);
        }
        json_object_put(alg_raw_stream_channel_num_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_RAW_STREAM_CHANNEL_NUM);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_RAW_STREAM_SAMPLE_RATE, &alg_raw_stream_sample_rate_object)) {
        if ((type = json_object_get_type(alg_raw_stream_sample_rate_object)) == json_type_int) {
            siren_config.raw_stream_config.raw_stream_sample_rate = json_object_get_int(alg_raw_stream_sample_rate_object);
            siren_printf(SIREN_INFO, "raw stream sample rate %d", siren_config.raw_stream_config.raw_stream_sample_rate);
        }
        json_object_put(alg_raw_stream_sample_rate_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_RAW_STREAM_SAMPLE_RATE);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    if (TRUE == json_object_object_get_ex(alg_config, KEY_RAW_STREAM_BYTE, &alg_raw_stream_byte_object)) {
        if ((type = json_object_get_type(alg_raw_stream_byte_object)) == json_type_int) {
            siren_config.raw_stream_config.raw_stream_byte = json_object_get_int(alg_raw_stream_byte_object);
            siren_printf(SIREN_INFO, "raw stream byte %d", siren_config.raw_stream_config.raw_stream_byte);
        }
        json_object_put(alg_raw_stream_byte_object);
    } else {
        siren_printf(SIREN_WARNING, "cannot find key %s", KEY_RAW_STREAM_BYTE);
        return CONFIG_ERROR_PARSE_FAIL;
    }

    json_object_put(basic_config);
    json_object_put(alg_config);
    json_object_put(config_object);

    return CONFIG_OK;
}

void SirenConfigurationManager::updateConfigFile(bool &useRemoteConfig) {
    useRemoteConfig = false;
    //if user give a not null path, check if that config exist
    std::string config_file_name;
    if (validPath) {
        config_file_name = config_file_path;
    } else {
        config_file_name = "blacksirenremote.json";
    }

    std::string prefix_store_path(CONFIG_STORE_FILE_PATH);
    if (prefix_store_path.empty()) {
        siren_printf(SIREN_ERROR, "store path is empty");
        return;
    }

    std::string remote_config_file_path(prefix_store_path.append(config_file_name));
    siren_printf(SIREN_INFO, "save to %s", remote_config_file_path.c_str());
    FILE *f = fopen(remote_config_file_path.c_str(), "w");
    if (f == nullptr) {
        siren_printf(SIREN_ERROR, "cannot open file");
        return;
    }

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        std::string ip = getAddressByHostname("config.open.rokid.com");
        if (ip.empty()) {
            siren_printf(BlackSiren::SIREN_ERROR, "cannot find address for host name config.open.rokid.com");
            return;
        } else {
            siren_printf(BlackSiren::SIREN_INFO, "address is %s", ip.c_str());
        }

        siren_printf(SIREN_INFO, "use remote url %s", CONFIG_REMOTE_CONFIG_FILE_URL);
        curl_easy_setopt(curl, CURLOPT_URL, CONFIG_REMOTE_CONFIG_FILE_URL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)f);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            siren_printf(SIREN_ERROR, "perform curl failed with %s", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return;
        }

        curl_easy_cleanup(curl);
    } else {
        siren_printf(SIREN_ERROR, "init curl failed");
        return;
    }
    curl_global_cleanup();
    fflush(f);
    fclose(f);
    
    //load remote config
    siren_printf(SIREN_INFO, "load from %s", remote_config_file_path.c_str());
    std::ifstream istream(remote_config_file_path.c_str());
    if (!istream.good()) {
        siren_printf(SIREN_ERROR, "remote config file is empty or failed since network");
        return;
    }

    std::stringstream string_buffer;
    string_buffer << istream.rdbuf();
    std::string remote_config(string_buffer.str());
    std::cout<<remote_config.c_str()<<std::endl;
    if (CONFIG_OK != loadConfigFromJSON(remote_config, siren_config)) {
        siren_printf(SIREN_ERROR, "parse remote config file failed");
        return;
    }

    useRemoteConfig = true;
}

config_error_t SirenConfigurationManager::parseConfigFile() {
    config_error_t error_status = CONFIG_OK;
    siren_printf(SIREN_INFO, "validPath = %d, use path %s", validPath, config_file_path.empty() ? "null" : config_file_path.c_str());


    bool useRemote = false;
    updateConfigFile(useRemote);
    if (!useRemote) {
        siren_printf(SIREN_INFO, "use backup %s", CONFIG_BACKUP_FILE_PATH);
        std::ifstream istream(CONFIG_BACKUP_FILE_PATH);
        //load backup failed use legacy
        if (!istream.good()) {
            siren_printf(SIREN_ERROR, "config file is not exist or other error");
            error_status = CONFIG_ERROR_OPEN_FILE;
        }

        if (error_status == CONFIG_OK) {
            std::stringstream string_buffer;
            string_buffer << istream.rdbuf();
            std::string contents(string_buffer.str());
            std::cout<<contents.c_str()<<std::endl;
            error_status = loadConfigFromJSON(contents, siren_config);
            siren_printf(SIREN_INFO, "load config with %d", error_status);
        }

    }

    /* use legacy file */
#ifdef CONFIG_LEGACY_SIREN_TEST
    siren_config.alg_config.alg_lan = CONFIG_LAN_ZH;
    siren_config.alg_config.alg_legacy_dir = LEGACY_ALG_DIR_CN;
#else
    /* TODO: use config file */
#endif

    return error_status;
}


}
