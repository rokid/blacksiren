#ifndef LF_QUEUE_H
#define LF_QUEUE_H

#include <atomic>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "os.h"
#include "sutils.h"

namespace BlackSiren {

#define ERR_OVERFLOW -2

struct LFCounter {
    LFCounter(): val(0), waiters(0) {}
    ~LFCounter() {
    }

    LFCounter(const LFCounter &) = delete;
    LFCounter& operator=(const LFCounter &) = delete;

    int inc(struct timespec *timeout, bool block);
    int dec(struct timespec *timeout);
    void wake();
    void wake_if_needed();

    volatile int val;
    volatile int waiters;
};

struct LFItem {
    LFItem() {
        data_ = nullptr;
    }
    ~LFItem() {
        data_ = nullptr;
    }

    LFItem(const LFItem &) = delete;
    LFItem &operator=(const LFItem &) = delete;

    int push(void *data, struct timespec* end_time, bool block);
    int pop(void ** data, struct timespec *end_time);
    
    LFCounter counter_;
    void * volatile data_;
};


struct LFQueue {
    LFQueue(uint32_t len, void *buf=nullptr) :
        push_(0), pop_(0), item_(nullptr), allocated_(nullptr),
        queued_item_(0) {
        SIREN_ASSERT(len != 0);
        if (((~len + 1) & len) != len) {
            uint32_t position = 0;
            for (int i = len; i != 0; i >>= 1) {
                position++;
            }
            len = static_cast<uint32_t>(1 << position);
            siren_printf(SIREN_INFO, "change len to %d", len);
        }

        if (buf == nullptr) {
            buf = malloc(sizeof (struct LFItem) * len);
            memset(buf, 0, sizeof(LFItem) * len);
            allocated_ = (void *)buf;
        }

        SIREN_ASSERT(buf != nullptr);
        item_ = (LFItem *)buf;
        pos_mask_ = len - 1;
    }

    ~LFQueue() {
        if (allocated_ == nullptr) {
            return;
        }

        free (allocated_);
    }

    LFQueue(const LFQueue &) = delete;
    LFQueue& operator=(const LFQueue &) = delete;

    int push(void *data);
    int pop(void** data, struct timespec *timeout);

    uint32_t remain();
    void reset();

    volatile unsigned long push_;
    volatile unsigned long pop_;
    unsigned long pos_mask_;
    LFItem * item_;
    void *allocated_;
    int queued_item_;
};

}

#endif
