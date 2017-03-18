#ifndef BLACK_SIREN_UTILS_H
#define BLACK_SIREN_UTILS_H

#include <cassert>

#include "common.h"

__BEGIN_DECLS

namespace BlackSiren {

enum {
    SIREN_DEBUG,
    SIREN_INFO,
    SIREN_WARNING,
    SIREN_ERROR
};

#define SIREN_DEBUG_STR "debug"
#define SIREN_INFO_STR "info"
#define SIREN_WARNING_STR "warning"
#define SIREN_ERROR_STR "error"

void set_sig_child_handler();
void unset_sig_child_handler();



#ifdef CONFIG_NO_STDOUT_DEBUG
#define siren_debug_print_timestamp do{}while(0)
#define siren_printf(args...) do{}while(0)

#else /* CONFIG_NO_STDOUT_DEBUG */

void siren_debug_print_timestamp(void);
void siren_printf(int level, const char *fmt, ...)
PRINTF_FORMAT(2, 3);

#endif


#ifdef CONFIG_NO_STDOUT_DEBUG
#define SIREN_ASSERT(a) dp{}while(0)
#else
#define SIREN_ASSERT(a)             \
    do {                            \
        if (!(a)) {                 \
            siren_printf(SIREN_ERROR, \
                    "SIREN_ASSERT FAILED '" #a "' " \
                    "%s %s:%d\n",   \
                    __FUNCTION__, __FILE__, __LINE__);  \
            assert(a);               \
        }                           \
    } while (0)
#endif
}

__END_DECLS

#endif
