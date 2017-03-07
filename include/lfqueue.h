#ifndef LF_QUEUE_H
#define LF_QUEUE_H

#include <atomic>
#include <time.h>
#include <stdint.h>

#include "common.h"
#include "os.h"
#include "sutils.h"

namespace BlackSiren {

#define ERR_OVERFLOW -2

struct LFCounter {
public :
    LFCounter(): _val(0), val(0), waiters(0) {}
    ~LFCounter() {
    }

    LFCounter(const LFCounter &) = delete;
    LFCounter& operator=(const LFCounter &) = delete;

    int inc(struct timespec *timeout, bool block);
    int dec(struct timespec *timeout);
    void wake();
    void wake_if_needed();

    friend int dec_if_gt0(LFCounter&);
    friend int inc_if_le0(LFCounter&);
private :
    volatile int _val;
    std::atomic_int val;
    std::atomic_int waiters;
};

struct LFItem {
public:
    LFItem() {
        _data = nullptr;
    }
    ~LFItem() {
        _data = nullptr;
    }

    LFItem(const LFItem &) = delete;
    LFItem &operator=(const LFItem &) = delete;

    int push(void *data, struct timespec* end_time, bool block);
    int pop(void ** data, struct timespec *end_time);
private:
    LFCounter counter;
    std::atomic<void *> _data;
};


class LFQueue {
public:
    LFQueue(uint32_t len, void *buf) :
        push_(0), pop_(0), item(nullptr), allocated(nullptr),
        queued_item(0) {
        SIREN_ASSERT(len != 0);
        if (((~len + 1) & len) != len) {
            uint32_t position = 0;
            for (int i = len; i != 0; i >>= 1) {
                position++;
            }
            len = static_cast<uint32_t>(1 << position);
        }

        if (buf == nullptr) {
            buf = malloc(sizeof (struct LFItem) * len);
            allocated = (void *)buf;
        }

        SIREN_ASSERT(buf != nullptr);
        item = (struct LFItem *)buf;
        pos_mask = len - 1;
    }

    ~LFQueue() {
        if (allocated == nullptr) {
            return;
        }

        free (allocated);
    }

    LFQueue(const LFQueue &) = delete;
    LFQueue& operator=(const LFQueue &) = delete;

    int push(void *data);
    int pop(void** data, struct timespec *timeout);

    uint32_t remain();
    void reset();

private:
    std::atomic_ulong push_;
    std::atomic_ulong pop_;
    uint32_t pos_mask;
    LFItem * item;
    void *allocated;
    std::atomic_int queued_item;
};

}

#endif
