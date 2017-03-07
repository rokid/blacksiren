#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>

#include <thread>
#include <atomic>
#include <functional>

#include "sutils.h"
#include "siren.h"
#include "isiren.h"
#include "siren_proxy.h"

using BlackSiren::siren_printf;
using BlackSiren::ISiren;
using BlackSiren::SirenProxy;

static bool check_nullptr_input_interface(siren_input_if_t *interface) {
    if (interface->init_input == nullptr ||
            interface->release_input == nullptr ||
            interface->start_input == nullptr ||
            interface->stop_input == nullptr ||
            interface->read_input == nullptr ||
            interface->on_err_input == nullptr) {
        return true;
    } else {
        return false;
    }
}

static ISiren& get_global_siren() {
    thread_local SirenProxy global_siren;
    return global_siren;
} 

siren_status_t init_siren(const char *path, siren_input_if_t *input) {
    if (path == nullptr) {
        siren_printf(BlackSiren::SIREN_WARNING, "empty json path use default settings");
    }
    
    if (input == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "empty input interface");
        return SIREN_STATUS_ERROR;
    }
   
    if (check_nullptr_input_interface(input)) {
        siren_printf(BlackSiren::SIREN_ERROR, "any callback in input interface is nullptr");
        return SIREN_STATUS_ERROR;
    }
    
    ISiren& siren = std::ref(get_global_siren());
        

    return SIREN_STATUS_OK;
}

void start_siren_process_stream(siren_proc_callback_t *callback) {

}

void start_siren_raw_stream(siren_raw_stream_callback_t *callback) {

}

void stop_siren_process_stream() {

}

void stop_siren_raw_stream() {

}

void stop_siren_stream() {

}

void set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) {

}

void set_siren_steer(float ho, float ver) {

}

void destroy_siren() {

}

siren_status_t rebuild_vt_word_list(const char **vt_word_list) {

    return SIREN_STATUS_OK;
}
