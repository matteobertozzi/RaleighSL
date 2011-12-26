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

#ifndef _Z_THREAD_H_
#define _Z_THREAD_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/types.h>

#include <pthread.h>

#ifdef __APPLE__
    #include <libkern/OSAtomic.h>
#endif

#if __APPLE__
    typedef OSSpinLock z_spinlock_t;
#elif defined(Z_ATOMIC_GCC)
    typedef union {
        volatile unsigned int data;
        struct {
            volatile unsigned short next_ticket;
            volatile unsigned short now_serving;
        } s;
    } z_spinlock_t;
#elif defined(Z_PTHREAD_HAS_SPIN)
    typedef pthread_spinlock_t z_spinlock_t;
#else
    #error "No SpinLock defined!"
#endif

#if defined(Z_PTHREAD_HAS_MUTEX)
    typedef pthread_mutex_t z_mutex_t;
#else
    #error "No Mutex defined!"
#endif

#if defined(Z_PTHREAD_HAS_RWLOCK)
    typedef pthread_rwlock_t z_rwlock_t;
#else
    #error "No RWLock defined!"
#endif

#if defined(Z_PTHREAD_HAS_COND)
    typedef pthread_cond_t z_cond_t;
#else
    #error "No Cond defined!"
#endif

#if defined(Z_PTHREAD_HAS_THREAD)
    typedef pthread_t z_thread_t;
#else
    #error "No Thread defined!"
#endif

int         z_spin_init             (z_spinlock_t *lock);
int         z_spin_destroy          (z_spinlock_t *lock);
int         z_spin_lock             (z_spinlock_t *lock);
int         z_spin_unlock           (z_spinlock_t *lock);

int         z_rwlock_init           (z_rwlock_t *lock);
int         z_rwlock_destroy        (z_rwlock_t *lock);
int         z_rwlock_rdlock         (z_rwlock_t *lock);
int         z_rwlock_wrlock         (z_rwlock_t *lock);
int         z_rwlock_unlock         (z_rwlock_t *lock);

int         z_mutex_init            (z_mutex_t *lock);
int         z_mutex_destroy         (z_mutex_t *lock);
int         z_mutex_lock            (z_mutex_t *lock);
int         z_mutex_unlock          (z_mutex_t *lock);

int         z_cond_init             (z_cond_t *cond);
int         z_cond_destroy          (z_cond_t *cond);
int         z_cond_wait             (z_cond_t *cond,
                                     z_mutex_t *mutex);
int         z_cond_timed_wait       (z_cond_t *cond,
                                     z_mutex_t *mutex,
                                     unsigned int time);
int         z_cond_broadcast        (z_cond_t *cond);
int         z_cond_signal           (z_cond_t *cond);

   
int         z_thread_create         (z_thread_t *thread,
                                     void * (*func) (void *),
                                     void *context);
int         z_thread_join           (z_thread_t *thread);
z_thread_t  z_thread_self           (void);

__Z_END_DECLS__

#endif /* !_Z_THREAD_H_ */

