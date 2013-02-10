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

#ifndef _Z_THREADING_H_
#define _Z_THREADING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memory.h>
#include <zcl/atomic.h>

#define Z_SYS_HAS_PTHREAD

#if defined(Z_SYS_HAS_PTHREAD)
    #include <pthread.h>
#endif

#if defined(Z_SYS_HAS_OS_SPIN_LOCK)
    #include <libkern/OSAtomic.h>
#endif

typedef void (*z_task_func_t) (void *data);

#define Z_THREAD_POOL(x)            Z_CAST(z_thread_pool_t, x)

Z_TYPEDEF_STRUCT(z_thread_pool)

#if defined(Z_SYS_HAS_PTHREAD_SPIN_LOCK)
    #define z_spinlock_t              pthread_spinlock_t
    #define z_spin_alloc(lock)        pthread_spin_init(lock, 0)
    #define z_spin_free(lock)         pthread_spin_destroy(lock)
    #define z_spin_lock(lock)         pthread_spin_lock(lock)
    #define z_spin_unlock(lock)       pthread_spin_unlock(lock)
#elif defined(Z_SYS_HAS_OS_SPIN_LOCK)
    #define z_spinlock_t              OSSpinLock
    #define z_spin_alloc(lock)        (*(lock) = OS_SPINLOCK_INIT)
    #define z_spin_free(lock)         (0)
    #define z_spin_lock(lock)         OSSpinLockLock(lock)
    #define z_spin_unlock(lock)       OSSpinLockUnlock(lock)
#else
    #error "No spinlock support"
#endif

#if defined(Z_SYS_HAS_PTHREAD_WAIT_COND)
    typedef struct z_wait_cond {
        pthread_mutex_t wlock;
        pthread_cond_t  wcond;
    } z_wait_cond_t;

    int     z_wait_cond_alloc       (z_wait_cond_t *wcond);
    void    z_wait_cond_free        (z_wait_cond_t *wcond);
    int     z_wait_cond_wait        (z_wait_cond_t *wcond, unsigned long msec);
    int     z_wait_cond_signal      (z_wait_cond_t *wcond);
    int     z_wait_cond_broadcast   (z_wait_cond_t *wcond);
#else
    #error "No wait condition"
#endif

#if defined(Z_SYS_HAS_PTHREAD)
    #define z_thread_t                pthread_t

    #define z_thread_alloc(thread, func, args)                              \
        pthread_create(thread, NULL, func, args)

    #define z_thread_join(thread)     pthread_join(*thread, NULL)
#endif

struct z_thread_pool {
    z_memory_t *      memory;
    z_atomic_queue_t *queue;
    unsigned int      nthreads;
    unsigned int      running;
    z_wait_cond_t     wcond;
    z_thread_t        threads[0];
};

z_thread_pool_t *   z_thread_pool_alloc         (z_memory_t *memory,
                                                 unsigned int nthreads);
void                z_thread_pool_free          (z_thread_pool_t *pool);

int                 z_thread_pool_add           (z_thread_pool_t *pool,
                                                 z_task_func_t func,
                                                 void *data);
int                 z_thread_pool_add_notify    (z_thread_pool_t *pool,
                                                 z_task_func_t func,
                                                 void *data,
                                                 z_task_func_t notify,
                                                 void *notify_data);

__Z_END_DECLS__

#endif /* !_Z_THREADING_H_ */
