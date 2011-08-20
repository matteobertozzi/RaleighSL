/*
 *   Copyright 2011 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef _Z_WORK_QUEUE_H_
#define _Z_WORK_QUEUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/thread.h>
#include <zcl/types.h>

#define Z_WORK_QUEUE(x)                     Z_CAST(z_work_queue_t, x)
#define Z_WORK_ITEM(x)                      Z_CAST(z_work_item_t, x)

Z_TYPEDEF_STRUCT(z_work_queue)
Z_TYPEDEF_STRUCT(z_work_item)

typedef int (*z_work_fetch_t)               (void *user_data,
                                             void **context,
                                             z_invoker_t *func);

struct z_work_item {
    z_work_item_t *next;
    z_work_item_t *prev;

    void *         context;
    z_invoker_t    func;
    int            fd;
};

struct z_work_queue {
    Z_OBJECT_TYPE

    union {
        struct wq_queue {
            z_work_item_t *head;
            z_work_item_t *tail;
        } queue;
        struct wq_fetch {
            z_work_fetch_t func;
            void *         user_data;
        } fetch;
    } internal;

    z_thread_t    *     threads;
    z_mutex_t        	wlock;
    z_cond_t        	wcond;
    z_mutex_t           qlock;
    unsigned int        ncore;
    int                 flags;
};

z_work_queue_t *    z_work_queue_alloc      (z_work_queue_t *queue,
                                             z_memory_t *memory,
                                             z_work_fetch_t fetch_func,
                                             void *user_data);
void                z_work_queue_free       (z_work_queue_t *queue);

void                z_work_queue_clear      (z_work_queue_t *queue);
int                 z_work_queue_add        (z_work_queue_t *queue,
                                             z_invoker_t func,
                                             void *context);
int                 z_work_queue_remove     (z_work_queue_t *queue,
                                             int item);

int                 z_work_queue_notify     (z_work_queue_t *queue);

int                 z_work_queue_stop       (z_work_queue_t *queue);
int                 z_work_queue_wait       (z_work_queue_t *queue);
int                 z_work_queue_loop       (z_work_queue_t *queue);

unsigned int        z_work_queue_id         (z_work_queue_t *queue);

__Z_END_DECLS__

#endif /* !_Z_WORK_QUEUE_H_ */

