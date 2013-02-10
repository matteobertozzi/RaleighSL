/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

#include <zcl/threading.h>

typedef struct z_task {
    z_task_func_t func;
    void *data;
    z_task_func_t notify;
    void *notify_data;
} z_task_t;

static z_task_t *__task_alloc (z_thread_pool_t *pool,
                               z_task_func_t func,
                               void *data,
                               z_task_func_t notify,
                               void *notify_data)
{
    return(NULL);
}

static void __task_free (z_thread_pool_t *pool, z_task_t *task) {
}

static int __task_push (z_thread_pool_t *pool, z_task_t *task) {
    return(0);
}

static z_task_t *__task_fetch (z_thread_pool_t *pool) {
    return(NULL);
}

static void *__worker_thread_loop (void *args) {
    z_thread_pool_t *pool = Z_THREAD_POOL(args);
    z_task_t *task;
    while (pool->running) {
        if ((task = __task_fetch(pool)) != NULL) {
            task->func(task->data);
            if (task->notify != NULL)
                task->notify(task->notify_data);
        } else {
            z_wait_cond_wait(&(pool->wcond), 0);
        }
    }
    return(NULL);
}

static int __thread_pool_add_workers (z_thread_pool_t *pool,
                                      unsigned int nthreads)
{
    while (nthreads--) {
        z_thread_t *thread = &(pool->threads[pool->nthreads]);
        if (!z_thread_alloc(thread, __worker_thread_loop, pool)) {
            pool->running = 0;
            while (pool->nthreads--) {
                thread = &(pool->threads[pool->nthreads]);
                z_thread_join(thread);
            }
            return(1);
        }
        pool->nthreads++;
    }
    return(0);
}

int z_thread_pool_add_notify (z_thread_pool_t *pool,
                              z_task_func_t func,
                              void *data,
                              z_task_func_t notify,
                              void *notify_data)
{
    z_task_t *task;

    if ((task = __task_alloc(pool, func, data, notify, notify_data)) == NULL)
        return(-1);

    if (__task_push(pool, task)) {
        __task_free(pool, task);
        return(-1);
    }

    z_wait_cond_signal(&(pool->wcond));
    return(0);
}


int z_thread_pool_add (z_thread_pool_t *pool, z_task_func_t func, void *data) {
    return(z_thread_pool_add_notify(pool, func, data, NULL, NULL));
}


z_thread_pool_t *z_thread_pool_alloc (z_memory_t *memory, unsigned int nthreads)
{
    z_thread_pool_t *pool;
    unsigned int size;

    /* Allocate thread-pool */
    size = sizeof(z_thread_pool_t) + ((nthreads) * sizeof(z_thread_t));
    if ((pool = z_memory_alloc(memory, z_thread_pool_t, size)) == NULL)
        return(NULL);

    /* Initialize thread-pool */
    pool->memory = memory;
    pool->nthreads = 0;
    pool->running = 1;

    /* Initialize queue */
    if ((pool->queue = z_atomic_queue_alloc(memory)) == NULL) {
        z_memory_free(memory, pool);
        return(NULL);
    }

    /* Initialize wait-condition */
    if (z_wait_cond_alloc(&(pool->wcond))) {
        z_atomic_queue_free(pool->queue);
        z_memory_free(memory, pool);
        return(NULL);
    }

    /* Initialize threads */
    if (__thread_pool_add_workers(pool, nthreads)) {
        z_wait_cond_free(&(pool->wcond));
        z_atomic_queue_free(pool->queue);
        z_memory_free(memory, pool);
        return(NULL);
    }

    return(pool);
}

void z_thread_pool_free (z_thread_pool_t *pool) {
    unsigned int nthreads;

    /* Stop workers */
    pool->running = 0;
    z_wait_cond_broadcast(&(pool->wcond));

    /* Wait threads to finish */
    nthreads = pool->nthreads;
    while (nthreads--) {
        z_thread_join(&(pool->threads[nthreads]));
    }

    z_wait_cond_free(&(pool->wcond));
    z_memory_free(pool->memory, pool);
}
