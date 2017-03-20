
#include <stdint.h>

#include <string>
#include <fstream>
#include <sstream>

#include "sutils.h"
#include "siren_config.h"

#ifdef CONFIG_LEGACY_SIREN_TEST

#define LEGACY_ALG_DIR_EN "/system/workdir_en"
#define LEGACY_ALG_DIR_CN "/system/workdir_cn"

#endif

namespace BlackSiren {

config_error_t SirenConfigurationManager::parseConfigFile() {
    config_error_t error_status = CONFIG_OK;
    if (validPath) {
        if (config_file_path.empty()) {
            siren_printf(SIREN_ERROR, "config file path is empty!");
            error_status = CONFIG_ERROR_OPEN_FILE;
        }

        if (error_status != CONFIG_OK) {
            std::ifstream istream(config_file_path);
            if (!istream.good()) {
                siren_printf(SIREN_ERROR, "config file is not exist or other error");
                error_status = CONFIG_ERROR_OPEN_FILE;
            }

            if (error_status != CONFIG_OK) {
                std::stringstream string_buffer;
                string_buffer << istream.rdbuf();
                std::string contents(string_buffer.str());
            }
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
