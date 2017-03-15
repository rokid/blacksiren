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

siren_status_t init_siren(void *token, const char *path, siren_input_if_t *input) {
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
    if (siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "init twice, please destory it first!");
        return SIREN_STATUS_ERROR;
    }

    return siren.init_siren(token, path, input);
}

void start_siren_process_stream(siren_proc_callback_t *callback) {
    if (callback == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "empty proc callback");
        return;
    }

    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return;
    }

    siren.start_siren_process_stream(callback);
}

void start_siren_raw_stream(siren_raw_stream_callback_t *callback) {
    if (callback == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "empty raw callback");
        return;
    }

    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return;
    }

    siren.start_siren_raw_stream(callback);
}

void stop_siren_process_stream() {
    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return;
    }
    siren.stop_siren_process_stream();
}

void stop_siren_raw_stream() {
    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return;
    }
    siren.stop_siren_raw_stream();
}

void stop_siren_stream() {
    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return;
    }
    siren.stop_siren_stream();
}

void set_siren_state(siren_state_t state, siren_state_changed_callback_t *callback) {
    if (callback == nullptr) {
        siren_printf(BlackSiren::SIREN_INFO, "use sync version");
    }

    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return;
    }

    siren.set_siren_state(state, callback);
}

void set_siren_steer(float ho, float ver) {
    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return;
    }

    siren.set_siren_steer(ho, ver);
}

void destroy_siren() {
    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return;
    }

    siren.destroy_siren();
}

siren_status_t rebuild_vt_word_list(const char **vt_word_list, int num) {
    if (vt_word_list == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "vt word list is nulltpr");
        return SIREN_STATUS_ERROR;
    }
    ISiren& siren = std::ref(get_global_siren());
    if (!siren.get_thread_key()) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren not init");
        return SIREN_STATUS_ERROR;
    }

    return siren.rebuild_vt_word_list(vt_word_list, num);
}
