#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "../include/sutils.h"
#include "../include/os.h"

namespace BlackSiren {

static void sig_child_handler(int sig) {
    siren_printf(SIREN_INFO, "received sig %d", sig);
    pid_t pid;
    int status;

    int saved_errno = errno;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status)) {
                siren_printf(SIREN_INFO, "Process %d exited cleanly (%d)", pid, WEXITSTATUS(status));
            }
        }  else if (WIFSIGNALED(status)) {
            if (WTERMSIG(status) != SIGKILL) {
                siren_printf(SIREN_INFO, "Process %d exited due to signal (%d)", pid, WTERMSIG(status));
            }
            if (WCOREDUMP(status)) {
                siren_printf(SIREN_ERROR, "Process %d dumped core", pid);
            }
        }
    }

    errno = saved_errno;
}

void set_sig_child_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_child_handler;

    int err = sigaction(SIGCHLD, &sa, NULL);
    if (err < 0) {
        siren_printf(SIREN_ERROR, "Error settings SIGCHLD handler: %s", strerror(errno));
    }
}

void unset_sig_child_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;

    int err = sigaction(SIGCHLD, &sa, NULL);
    if (err < 0) {
        siren_printf(SIREN_ERROR, "Error settings SIGCHLD handler: %s", strerror(errno));
    }
}

#ifdef CONFIG_ANDROID_LOG
static int siren_to_android_level(int level) {
    if (level == SIREN_DEBUG) {
        return ANDROID_LOG_DEBUG;
    } else if (level == SIREN_INFO) {
        return ANDROID_LOG_INFO;
    } else if (level == SIREN_WARNING) {
        return ANDROID_LOG_WARN;
    } else if (level == SIREN_ERROR) {
        return ANDROID_LOG_ERROR;
    } else {
        return ANDROID_LOG_INFO;
    }
}
#endif

void siren_debug_print_timestamp() {
    /* Fix: use os file to make it ok to porting to other OSs
     */
    struct os_time tv;
    os_get_time(&tv);
    printf ("[%ld.%06u]: ", (long) tv.sec, (unsigned int) tv.usec);
}

void siren_printf(int level, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    {
#ifdef CONFIG_ANDROID_LOG
        __android_log_vprint(
            siren_to_android_level(level),
            ANDROID_LOG_TAG, fmt, ap);
#endif
        SIREN_UNUSED(level);
        siren_debug_print_timestamp();
        vprintf(fmt, ap);
        printf("\n");
    }
    va_end(ap);
}

}
