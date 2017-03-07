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

static inline int dec_if_gt0(std::atomic_int &val, volatile int *p) {
    int x = 0;
    while ((x = val.load(std::memory_order_acquire)) > 0 &&
            !val.compare_exchange_weak(x, x - 1,
                                       std::memory_order_acq_rel, std::memory_order_relaxed));
    *p = val.load();
    return x;
}

static inline int inc_if_le0(std::atomic_int &val, volatile int *p) {
    int x = 0;
    while ((x = val.load(std::memory_order_acquire)) <= 0 &&
            !val.compare_exchange_weak(x, x + 1,
                                       std::memory_order_acq_rel, std::memory_order_relaxed));
    *p = val.load();
    return x;
}

int dec_if_gt0(LFCounter &counter) {
    return dec_if_gt0(counter.val, &counter._val);
}

int inc_if_le0(LFCounter &counter) {
    return inc_if_le0(counter.val, &counter._val);
}

int LFCounter::inc(struct timespec *timeout, bool block) {
    int err = 0;
    int val_ = 0;
    if ((val_ = inc_if_le0(*this)) <= 0) {
    } else if (!block) {
        val_ = val.fetch_add(1);
        _val = val.load();
    } else {
        waiters.fetch_add(1, std::memory_order_relaxed);
        while (1) {
            if (-110 == (err = futex_wait(&this->_val, val_, timeout))) {
                val_ = val.fetch_add(1);
                _val = val.load();
                break;
            }
            if ((val_ = inc_if_le0(*this)) <= 0) {
                break;
            }
        } 
        waiters.fetch_add(-1, std::memory_order_relaxed);
    }
    return val_;
}

int LFCounter::dec(struct timespec *timeout) {
    int err = 0;
    int val_ = 0;
    if ((val_ = dec_if_gt0(*this)) > 0) {
    } else {
        waiters.fetch_add(1, std::memory_order_relaxed);
        while (1) {
            if (-ETIMEDOUT == (err = futex_wait(&this->_val, val_, timeout))) {
                val_ = val.fetch_add(-1);
                _val = val.load();
            }
            if ((val_ = dec_if_gt0(*this)) > 0) {
                break;
            }
        }
        waiters.fetch_add(-1, std::memory_order_relaxed);
    }
    return val_;
}

void LFCounter::wake() {
    futex_wake(&_val, INT32_MAX);
}

void LFCounter::wake_if_needed() {
    if (waiters.load(std::memory_order_relaxed) > 0) {
        wake();
    }
}

int LFItem::push( void *pdata, struct timespec *end_time, bool block) {
    int err = 0;
    int old_val = 0;
    if ((old_val = counter.inc(end_time, block)) < 0) {
        counter.wake_if_needed();
        err = -1;
    } else if (old_val > 0) {
        counter.wake_if_needed();
        err = -2;
    } else {
        void *t = nullptr;
        while (!_data.compare_exchange_weak(t, pdata,
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed));
        counter.wake_if_needed();
    }
    return err;
}

int LFItem::pop( void **data, struct timespec *end_time) {
    int err = 0;
    void *data_ = nullptr;
    if (counter.dec(end_time) != 1) {
        std::cout<<"dec != 1"<<std::endl;
        counter.wake_if_needed();
        err = -1;
    } else {
        void *t = nullptr;
        counter.wake_if_needed();
        while ((nullptr == (data_ = _data.load(std::memory_order_acquire)))
                || !_data.compare_exchange_weak(data_, t,
                                                std::memory_order_acq_rel,
                                                std::memory_order_relaxed));
    }
    *data = data_;
    return err;
}

int LFQueue::push (void *data) {
    int err = 0;
    while (1) {
        unsigned long seq = push_.fetch_add(1, std::memory_order_relaxed);
        LFItem *it = item + (seq & pos_mask);
        if (0 == it->push(data, NULL, 0)) {
            queued_item.fetch_add(1, std::memory_order_relaxed);
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
    unsigned long seq = pop_.fetch_add(1, std::memory_order_relaxed);
    LFItem *it = item + (seq & pos_mask);
    if (0 == it->pop(data, timeout)) {
        queued_item.fetch_add(-1, std::memory_order_relaxed);
    }

    return err;
}

uint32_t LFQueue::remain() {
    return push_ - pop_;
}

void LFQueue::reset() {
    while (queued_item.load(std::memory_order_relaxed) > 0) {
        void *p;
        pop(&p, nullptr);
    }
}

}
