/*
 * easyr2_queue.h
 *
 *  Created on: 2014-9-5
 *      Author: root
 */

#ifndef EASYR2_QUEUE_H_
#define EASYR2_QUEUE_H_

#include <unistd.h>
#include "common.h"

__BEGIN_DECLS

namespace BlackSiren {

struct easyr2_counter {
    volatile int val;
    volatile int waiters;
};

struct easyr2_queue_item {
    struct easyr2_counter counter_;
    void* volatile data_;
};

struct easyr2_queue {
    volatile unsigned long push_;
    volatile unsigned long pop_;
    unsigned long pos_mask_;
    struct easyr2_queue_item *items_;
    void * allocated_;
    int queued_item_;
};

int easyr2_queue_init(struct easyr2_queue *_this, unsigned long len,
                      void* buf);
int easyr2_queue_destroy(struct easyr2_queue *_this) ;
int easyr2_queue_push(struct easyr2_queue *_this, void* data);
int easyr2_queue_pop(struct easyr2_queue *_this, void** data,
                     struct timespec* timeout);
unsigned long easyr2_queue_remain(struct easyr2_queue *_this);
void easyr2_queue_reset(struct easyr2_queue *_this);
void init_queue_item(struct easyr2_queue_item *_this);

}

__END_DECLS

#endif /* EASYR2_QUEUE_H_ */
