/*
 * easyr2_queue.c
 *
 *  Created on: 2014-9-5
 *      Author: root
 */

#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/cdefs.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "easyr2_queue.h"



#define futex(...) syscall(SYS_futex,__VA_ARGS__)

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

inline int futex_wake(volatile int* p, int val) {
    int err = 0;
    if (0 != futex((int*)p, FUTEX_WAKE_PRIVATE, val, NULL, NULL, 0)) {
        err = errno;
    }
    return err;
}

inline int futex_wait(volatile int* p, int val, struct timespec* timeout) {
    int err = 0;
    if (0 != futex((int*)p, FUTEX_WAIT_PRIVATE, val, timeout, NULL, 0)) {
        err = errno;
    }
    return err;
}

static int dec_if_gt0(volatile int* p) {
    int x = 0;
    while ((x = *p) > 0 && !__sync_bool_compare_and_swap(p, x, x - 1))
        ;
    return x;
}

static int inc_if_le0(volatile int* p) {
    int x = 0;
    while ((x = *p) <= 0 && !__sync_bool_compare_and_swap(p, x, x + 1))
        ;
    return x;
}

static int counter_inc(struct easyr2_counter *thiz, struct timespec * timeout,
                       char block) {
    int err = 0;
    int32_t val = 0;
    //0->1,-1->0
    if ((val = inc_if_le0(&thiz->val)) <= 0) {
    } else if (!block) {
        val = __sync_fetch_and_add(&thiz->val, 1);
    } else {
        // push > pop
        __sync_add_and_fetch(&thiz->waiters, 1);
        while (1) {
            //wait
            if (-110 == (err = futex_wait(&thiz->val, val, timeout))) {
                val = __sync_fetch_and_add(&thiz->val, 1);
                break;
            }
            //wake up or val is changed before futex_wait, futex_wait return directly
            if ((val = inc_if_le0(&thiz->val)) <= 0) {
                break;
            }
        }
        __sync_add_and_fetch(&thiz->waiters, -1);
    }
    return val;
}

static int counter_dec(struct easyr2_counter *thiz, struct timespec * timeout) {
    int err = 0;
    int32_t val = 0;
    if ((val = dec_if_gt0(&thiz->val)) > 0) {
    } else {
        __sync_add_and_fetch(&thiz->waiters, 1);
        while (1) {
            if (-ETIMEDOUT == (err = futex_wait(&thiz->val, val, timeout))) {
                val = __sync_fetch_and_add(&thiz->val, -1);
                break;
            }
            if ((val = dec_if_gt0(&thiz->val)) > 0) {
                break;
            }
        }
        __sync_add_and_fetch(&thiz->waiters, -1);
    }
    return val;
}

static void wake(struct easyr2_counter *thiz) {
    futex_wake(&thiz->val, INT32_MAX);
}

static void counter_wake_if_needed(struct easyr2_counter *thiz) {
    if (thiz->waiters > 0) {
        wake(thiz);
    }
}

static void init_counter(struct easyr2_counter *thiz) {
    thiz->val = 0;
    thiz->waiters = 0;
}

void init_queue_item(struct easyr2_queue_item *thiz) {
    thiz->data_ = NULL;
    init_counter(&thiz->counter_);
}

static int queue_item_push(struct easyr2_queue_item *thiz, void* data,
                           struct timespec* end_time, char block) {
    int err = 0;
    int old_val = 0;
    if ((old_val = counter_inc(&thiz->counter_, end_time, block)) < 0) {
        counter_wake_if_needed(&thiz->counter_);
        err = -1;
    } else if (old_val > 0) {
        counter_wake_if_needed(&thiz->counter_);
        err = -2;
    } else {
        while (!__sync_bool_compare_and_swap(&thiz->data_, NULL, data))
            ;
        counter_wake_if_needed(&thiz->counter_);
    }
    return err;
}

static int queue_item_pop(struct easyr2_queue_item *thiz, void** data,
                          struct timespec* end_time) {
    int err = 0;
    void *data_ = NULL;
    //1->0
    if (counter_dec(&thiz->counter_, end_time) != 1) {
        counter_wake_if_needed(&thiz->counter_);
        err = -1;
    } else {
        counter_wake_if_needed(&thiz->counter_);
        while (NULL == (data_ = thiz->data_)
                || !__sync_bool_compare_and_swap(&thiz->data_, data_, NULL)) {
        }
    }
    *data = data_;
    return err;
}

static char is2n(unsigned long input) {
    return (((~input + 1) & input) == input);
}
;

int easyr2_queue_init(struct easyr2_queue *thiz, unsigned long len,
                      void* buf) {
    thiz->push_ = 0;
    thiz->pop_ = 0;
    thiz->pos_mask_ = 0;
    thiz->items_ = NULL;
    thiz->allocated_ = NULL;
    thiz->queued_item_ = 0;
    struct easyr2_queue_item *item = NULL;

    if (0 == len || !is2n(len)) {
        return -1;
    }

    if (NULL != thiz->items_) {
        return -2;
    }

    if (buf == NULL) {
        buf = malloc(sizeof (struct easyr2_queue_item) * len);
        thiz->allocated_ = (void *) buf;
    }

    if (buf == NULL) {
        return -3;
    }

    item = (struct easyr2_queue_item *) buf;
    thiz->pos_mask_ = len - 1;
    memset(item, 0, sizeof(struct easyr2_queue_item) * len);
    thiz->items_ = item;

    return 0;
}

int easyr2_queue_destroy(struct easyr2_queue *thiz) {
    if (thiz->allocated_ == NULL)
        return -1;
    free(thiz->allocated_);
    return 0;
}

int easyr2_queue_push(struct easyr2_queue *thiz, void* data) {
    int err = 0;
    while (1) {
        uint64_t seq = __sync_fetch_and_add(&thiz->push_, 1);
        struct easyr2_queue_item *pi = thiz->items_ + (seq & thiz->pos_mask_);
        if (0 == (err = queue_item_push(pi, data, NULL, 0))) {
            __sync_fetch_and_add(&thiz->queued_item_, 1);
            break;
        } else if (-1 == err) {
            continue;
        } else if (-2 == err) {
            break;
        }
    }
    return err;
}

int easyr2_queue_pop(struct easyr2_queue *thiz, void** data,
                     struct timespec* timeout) {
    int err = 0;
    uint64_t seq = __sync_fetch_and_add(&thiz->pop_, 1);
    struct easyr2_queue_item* pi = thiz->items_ + (seq & thiz->pos_mask_);
    if (0 == (err = queue_item_pop(pi, data, timeout))) {
        __sync_fetch_and_add(&thiz->queued_item_, -1);
    }
    return err;
}

unsigned long easyr2_queue_remain(struct easyr2_queue *thiz) {
    return thiz->push_ - thiz->pop_;
}

#if 0
int easyr2_queue_size(struct easyr2_queue *thiz) {
    return thiz->queued_item_;
}
#endif

void easyr2_queue_reset(struct easyr2_queue *thiz) {
    while (thiz->queued_item_ > 0) {
        void * p;
        easyr2_queue_pop(thiz, &p, NULL);
    }
}
}
