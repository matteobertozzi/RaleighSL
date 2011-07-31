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

#include <unistd.h>

#include <zcl/workqueue.h>
#include <zcl/system.h>

#define __WORKQ_STOPPED                 (1)
#define __WORKQ_WAITING                 (2)
#define __WORKQ_RUNNING                 (4)
#define __WORKQ_INTERNQ                 (8)

#define __WORKQ_STATE         (__WORKQ_STOPPED|__WORKQ_WAITING|__WORKQ_RUNNING)

#define __workq_set_running(queue)                                          \
    do {                                                                    \
        Z_WORK_QUEUE(queue)->flags &= ~__WORKQ_STATE;                       \
        Z_WORK_QUEUE(queue)->flags |= __WORKQ_RUNNING;                      \
    } while (0)

#define __workq_set_waiting(queue)                                          \
    do {                                                                    \
        Z_WORK_QUEUE(queue)->flags &= ~__WORKQ_STATE;                       \
        Z_WORK_QUEUE(queue)->flags |= __WORKQ_WAITING | __WORKQ_RUNNING;    \
    } while (0)

#define __workq_set_stopped(queue)                                          \
    do {                                                                    \
        Z_WORK_QUEUE(queue)->flags &= ~__WORKQ_STATE;                       \
        Z_WORK_QUEUE(queue)->flags |= __WORKQ_STOPPED;                      \
    } while (0)

static z_work_item_t *__work_item_alloc (z_work_queue_t *queue,
                                         z_invoker_t func,
                                         void *context)
{
    z_work_item_t *item;

    if ((item = z_object_struct_alloc(queue, z_work_item_t)) != NULL) {
        item->next = NULL;
        item->prev = NULL;
        item->func = func;
        item->context = context;
    }

    return(item);
}

static void __work_item_free (z_work_queue_t *queue,
                              z_work_item_t *item)
{
    z_object_struct_free(queue, z_work_item_t, item);
}

static z_work_item_t *__work_item_fetch (z_work_queue_t *queue,
                                         z_work_item_t *stack_item)
{
    z_work_item_t *next;
    z_work_item_t *item;

    z_mutex_lock(&(queue->qlock));
    if (queue->flags & __WORKQ_INTERNQ) {        
        if ((item = queue->internal.queue.head) != NULL) {
            if ((next = item->next) != NULL) {
                next->prev = NULL;
                queue->internal.queue.head = next;
            } else {
                queue->internal.queue.head = NULL;
                queue->internal.queue.tail = NULL;
            }
        }        
    } else {
        item = stack_item;
        if (queue->internal.fetch.func(queue->internal.fetch.user_data,
                                       &(item->context),
                                       &(item->func)))
        {
            item = NULL;
        }
    }
    z_mutex_unlock(&(queue->qlock));

    return(item);
}

static void *__work_queue_loop (void *queue) {
    z_work_item_t stkitem;
    z_work_item_t *item;
    z_invoker_t func;
    void *context;

    while (Z_WORK_QUEUE(queue)->flags & __WORKQ_RUNNING) {
        if ((item = __work_item_fetch(Z_WORK_QUEUE(queue), &stkitem)) == NULL) {
            if (Z_WORK_QUEUE(queue)->flags & __WORKQ_WAITING)
                return(NULL);

            z_mutex_lock(&(Z_WORK_QUEUE(queue)->wlock));
            z_cond_wait(&(Z_WORK_QUEUE(queue)->wcond),
                        &(Z_WORK_QUEUE(queue)->wlock));
            z_mutex_unlock(&(Z_WORK_QUEUE(queue)->wlock));
        } else {
            /* free operation item */
            context = item->context;
            func = item->func;

            if (item != &stkitem)
                __work_item_free(Z_WORK_QUEUE(queue), item);

            /* run job */
            func(context);
        }
    }

    return(NULL);
}

static int __work_queue_pool_alloc (z_work_queue_t *queue) {
    z_thread_t *tid;
    long ncore;

    if (z_mutex_init(&(queue->wlock)))
        return(-1);

    if (z_cond_init(&(queue->wcond))) {
        z_mutex_destroy(&(queue->wlock));
        return(-2);
    }

    ncore = z_system_processors();
    if (!(queue->threads = z_object_array_alloc(queue, ncore, z_thread_t))) {
        z_cond_destroy(&(queue->wcond));
        z_mutex_destroy(&(queue->wlock));
        return(-3);
    }

    __workq_set_running(queue);

    queue->ncore = ncore;
    while (ncore--) {
        tid = &(queue->threads[ncore]);
        if (z_thread_create(tid, __work_queue_loop, queue)) {
            /* Huston we've a failure, rollback! */
            __workq_set_stopped(queue);
            for (; ncore < queue->ncore; ++ncore)
                z_thread_join(&(queue->threads[ncore]));
            z_object_array_free(queue, queue->threads);
            return(-4);
        }
    }

    return(0);
}

z_work_queue_t *z_work_queue_alloc (z_work_queue_t *queue,
                                    z_memory_t *memory,
                                    z_work_fetch_t fetch_func,
                                    void *user_data)
{
    if ((queue = z_object_alloc(memory, queue, z_work_queue_t)) == NULL)
        return(NULL);

    queue->ncore = 0;
    queue->threads = NULL;
    queue->flags = __WORKQ_STOPPED;

    if (z_mutex_init(&(queue->qlock))) {
        z_object_free(queue);
        return(NULL);
    }

    if (fetch_func != NULL) {
        queue->internal.fetch.func = fetch_func;
        queue->internal.fetch.user_data = user_data;
    } else {
        queue->internal.queue.head = NULL;
        queue->internal.queue.tail = NULL;
        queue->flags |= __WORKQ_INTERNQ;
    }

    if (__work_queue_pool_alloc(queue)) {
        z_object_free(queue);
        return(NULL);
    }

    return(queue);
}

void z_work_queue_free (z_work_queue_t *queue) {
    if (!(queue->flags & __WORKQ_WAITING)) {
        z_work_queue_stop(queue);
        z_work_queue_clear(queue);
        z_work_queue_loop(queue);
    }

    z_object_array_free(queue, queue->threads);
    z_mutex_destroy(&(queue->wlock));
    z_cond_destroy(&(queue->wcond));
    z_mutex_destroy(&(queue->qlock));

    z_object_free(queue);
}

void z_work_queue_clear (z_work_queue_t *queue) {
    z_work_item_t *next;
    z_work_item_t *p;

    z_mutex_lock(&(queue->qlock));
    for (p = queue->internal.queue.head; p != NULL; p = p->next) {
        next = p->next;
        __work_item_free(queue, p);
    }

    queue->internal.queue.head = NULL;
    queue->internal.queue.tail = NULL;
    z_mutex_unlock(&(queue->qlock));
}

int z_work_queue_add (z_work_queue_t *queue,
                      z_invoker_t func,
                      void *context)
{
    z_work_item_t *item;
    z_work_item_t *tail;

    if (!(queue->flags & __WORKQ_INTERNQ))
        return(-1);

    if ((item = __work_item_alloc(queue, func, context)) == NULL)
        return(1);

    z_mutex_lock(&(queue->qlock));

    if ((tail = queue->internal.queue.tail) != NULL) {
        item->fd = tail->fd + 1;
        item->prev = tail;
        tail->next = item;
        queue->internal.queue.tail = item;
    } else {
        item->fd = 0;
        queue->internal.queue.head = item;
        queue->internal.queue.tail = item;
    }

    z_mutex_unlock(&(queue->qlock));
    z_cond_signal(&(queue->wcond));
    return(0);
}

int z_work_queue_remove (z_work_queue_t *queue,
                         int item)
{
    z_work_item_t *p;
    int fd;

    if (!(queue->flags & __WORKQ_INTERNQ))
        return(-1);

    z_mutex_lock(&(queue->qlock));

    fd = 0;
    for (p = queue->internal.queue.head; p != NULL; p = p->next) {
        if (item == p->fd) {
            z_work_item_t *x;

            if ((x = p->prev) != NULL)
                x->next = p->next;

            if ((x = p->next) != NULL)
                x->prev = p->prev;

            z_mutex_unlock(&(queue->qlock));

            __work_item_free(queue, p);
            return(0);
        }

        fd++;
    }

    z_mutex_unlock(&(queue->qlock));
    return(1);
}

int z_work_queue_notify (z_work_queue_t *queue) {
    z_cond_signal(&(queue->wcond));
    return(0);
}

int z_work_queue_stop (z_work_queue_t *queue) {
    if (!(queue->flags & __WORKQ_RUNNING))
        return(1);

    __workq_set_stopped(queue);
	z_cond_broadcast(&(queue->wcond));
    return(0);
}

int z_work_queue_wait (z_work_queue_t *queue) {
    __workq_set_waiting(queue);
	z_cond_broadcast(&(queue->wcond));
    return(z_work_queue_loop(queue));
}

int z_work_queue_loop (z_work_queue_t *queue) {
    int ncore;

    ncore = queue->ncore;
    while (ncore--)
        z_thread_join(&(queue->threads[ncore]));

    return(0);
}

unsigned int z_work_queue_id (z_work_queue_t *queue) {
    pthread_t thread;
    unsigned int i;

    thread = pthread_self();
    for (i = 0; i < queue->ncore; ++i) {
        if (thread == queue->threads[i])
            return(i);
    }

    /* Never Reached! */
    return(0);
}

