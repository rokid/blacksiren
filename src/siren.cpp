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


siren_t init_siren(void *token, const char *path, siren_input_if_t *input) {
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

    SirenProxy *proxy = new SirenProxy;
    if (proxy->init_siren(token, path, input) != SIREN_STATUS_OK) {
        delete proxy;
        return (unsigned long)nullptr;
    } else {
        return (unsigned long)proxy;
    }
}

void start_siren_process_stream(siren_t siren, siren_proc_callback_t *callback) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }

    if (callback == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "empty proc callback");
        return;
    }

    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->start_siren_process_stream(callback);
}

void start_siren_raw_stream(siren_t siren, siren_raw_stream_callback_t *callback) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }

    if (callback == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "empty raw callback");
        return;
    }

    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->start_siren_raw_stream(callback);
}

void stop_siren_process_stream(siren_t siren) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }

    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->stop_siren_process_stream();
}

void stop_siren_raw_stream(siren_t siren) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }

    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->stop_siren_raw_stream();
}

void stop_siren_stream(siren_t siren) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }

    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->stop_siren_stream();
}

void set_siren_state(siren_t siren, siren_state_t state, siren_state_changed_callback_t *callback) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }

    if (callback == nullptr) {
        siren_printf(BlackSiren::SIREN_INFO, "use sync version");
    }

    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->set_siren_state(state, callback);
}

void set_siren_steer(siren_t siren, float ho, float ver) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }
    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->set_siren_steer(ho, ver);
}

void destroy_siren(siren_t siren) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }
    SirenProxy *proxy = (SirenProxy *)siren;

    proxy->destroy_siren();
    delete proxy;
}

siren_status_t rebuild_vt_word_list(siren_t siren, const char **vt_word_list, int num) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return SIREN_STATUS_ERROR;
    }
    
    if (vt_word_list == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "vt word list is nulltpr");
        return SIREN_STATUS_ERROR;
    }
    
    SirenProxy *proxy = (SirenProxy *)siren;
    return proxy->rebuild_vt_word_list(vt_word_list, num);
}

void start_siren_monitor(siren_t siren, siren_net_callback_t *callback) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "sire is null");
        return;
    }
    
    if (callback == nullptr) {
        siren_printf(BlackSiren::SIREN_ERROR, "callback is nullptr");
        return;
    }

    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->start_siren_monitor(callback);
}

void broadcast_siren_event(siren_t siren, char *data, int len) {
    if (siren == 0) {
        siren_printf(BlackSiren::SIREN_ERROR, "siren is null");
        return;
    }

    SirenProxy *proxy = (SirenProxy *)siren;
    proxy->broadcast_siren_event(data, len);
}
