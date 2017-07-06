#ifndef SIREN_CONFIG_H_
#define SIREN_CONFIG_H_

#include <stdint.h>
#include <vector>
#include <string>

#include "siren_config_if.h"
namespace BlackSiren {

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
