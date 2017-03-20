#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <linux/futex.h>
#include <sys/cdefs.h>

#include <iostream>
#include <atomic>
#include <algorithm>

#include "sutils.h"
#include "lfqueue.h"

#ifndef futex
#define futex(...)  syscall(SYS_futex, __VA_ARGS__)
#endif

#ifndef FUTEX_WAIT
#define FUTEX_WAIT 0
#endif

#ifndef FUTEX_WAKE
#define FUTEX_WAKE 1
#endif

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif

#ifndef FUTEX_WAIT_PRIVATE
#define FUTEX_WAIT_PRIVATE (FUTEX_WAIT|FUTEX_PRIVATE_FLAG)
#endif

#ifndef FUTEX_WAKE_PRIVATE
#define FUTEX_WAKE_PRIVATE (FUTEX_WAKE|FUTEX_PRIVATE_FLAG)
#endif

namespace BlackSiren {

static inline int futex_wake(volatile int* p, int val) {
    int err = 0;
    if (0 != futex((int *)p, FUTEX_WAKE_PRIVATE, val, NULL, NULL, 0)) {
        err = errno;
    }
    return err;
}

static inline int futex_wait(volatile int* p, int val, struct timespec* timeout) {
    int err = 0;
    if (0 != futex((int *)p, FUTEX_WAIT_PRIVATE, val, timeout, NULL, 0)) {
        err = errno;
    }
    return err;
}

static inline int dec_if_gt0(volatile int *p) {
    int x = 0;
    while ((x = *p) > 0 &&
            !__sync_bool_compare_and_swap(p, x, x - 1));
    return x;
}

static inline int inc_if_le0(volatile int *p) {
    int x = 0;
    while ((x = *p) <= 0 &&
                !__sync_bool_compare_and_swap(p, x, x + 1));
    return x;
}

int LFCounter::inc(struct timespec *timeout, bool block) {
    int err = 0;
    int val = 0;
    if ((val = inc_if_le0(&this->val)) <= 0) {
    } else if (!block) {
        val = __sync_fetch_and_add(&this->val, 1);
    } else {
        __sync_add_and_fetch(&this->waiters, 1);
        while (1) {
            if (-110 == (err = futex_wait(&this->val, val, timeout))) {
                val = __sync_fetch_and_add(&this->val, 1);
                break;
            }
            if ((val = inc_if_le0(&this->val)) <= 0) {
                break;
            }
        } 
        __sync_add_and_fetch(&this->waiters, -1);
    }
    return val;
}

int LFCounter::dec(struct timespec *timeout) {
    int err = 0;
    int val = 0;
    if ((val = dec_if_gt0(&this->val)) > 0) {
    } else {
        __sync_add_and_fetch(&this->waiters, 1);
        while (1) {
            if (-ETIMEDOUT == (err = futex_wait(&this->val, val, timeout))) {
                val = __sync_fetch_and_add(&this->val, -1);
                break;
            }

            if ((val = dec_if_gt0(&this->val)) > 0) {
                break;
            }
        }
        __sync_add_and_fetch(&this->waiters, -1);
    }
    return val;
}

void LFCounter::wake() {
    futex_wake(&this->val, INT32_MAX);
}

void LFCounter::wake_if_needed() {
    if (waiters > 0) {
        wake();
    }
}

int LFItem::push( void *pdata, struct timespec *end_time, bool block) {
    int err = 0;
    int old_val = 0;
    if ((old_val = counter_.inc(end_time, block)) < 0) {
        counter_.wake_if_needed();
        err = -1;
    } else if (old_val > 0) {
        counter_.wake_if_needed();
        err = -2;
    } else {
        while (!__sync_bool_compare_and_swap(&this->data_, nullptr, pdata));
        counter_.wake_if_needed();
    }
    return err;
}

int LFItem::pop( void **data, struct timespec *end_time) {
    int err = 0;
    void *data_ = nullptr;
    if (counter_.dec(end_time) != 1) {
        counter_.wake_if_needed();
        err = -1;
    } else {
        counter_.wake_if_needed();
        while ((nullptr == (data_ = this->data_)
                || !__sync_bool_compare_and_swap(&this->data_, data_, nullptr))) {
        }
    }
    *data = data_;
    return err;
}

int LFQueue::push (void *data) {
    int err = 0;
    while (1) {
        unsigned long long seq = __sync_fetch_and_add(&push_, 1);
        LFItem *it = item_ + (seq & pos_mask_);
        if (0 == (err = it->push(data, NULL, 0))) {
            __sync_fetch_and_add(&queued_item_, 1);
            break;
        } else if (-1 == err) {
            continue;
        } else if (-2 == err) {
            //overflow
            break;
        }
    }
    return 0;
}

int LFQueue::pop (void **data, struct timespec *timeout) {
    int err = 0;
    unsigned long long seq = __sync_fetch_and_add(&pop_, 1);
    LFItem *it = item_ + (seq & pos_mask_);
    if (0 == (err = it->pop(data, timeout))) {
        __sync_fetch_and_add(&queued_item_, -1);
    }

    return err;
}

uint32_t LFQueue::remain() {
    return push_ - pop_;
}

void LFQueue::reset() {
    while (queued_item_ > 0) {
        void *p;
        pop(&p, nullptr);
    }
}

}
