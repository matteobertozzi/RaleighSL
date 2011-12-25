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

#include <zcl/thread.h>

#if defined(Z_ASM_HAS_PAUSE)
    #define __cpu_relax()     asm volatile("pause\n": : :"memory")
#else
    #define __cpu_relax()     while (0)
#endif

int z_spin_init (z_spinlock_t *lock) {
#if __APPLE__
    *lock = OS_SPINLOCK_INIT;
    return(0);
#elif defined(Z_ATOMIC_GCC)
    lock->s.now_serving = 0;
    lock->s.next_ticket = 0;
    return(0);
#elif defined(Z_PTHREAD_HAS_SPIN)
    return(pthread_spin_init((pthread_spinlock_t *)lock,
                             PTHREAD_PROCESS_PRIVATE));
#endif
    return(-1);
}

int z_spin_destroy (z_spinlock_t *lock) {
#if __APPLE__
    return(0);
#elif defined(Z_ATOMIC_GCC)
    return(0);
#elif defined(Z_PTHREAD_HAS_SPIN)
    return(pthread_spin_destroy((pthread_spinlock_t *)lock));
#endif
    return(-1);
}

int z_spin_lock (z_spinlock_t *lock) {
#if __APPLE__
    OSSpinLockLock((OSSpinLock *)lock);
    return(0);
#elif defined(Z_ATOMIC_GCC)
    unsigned short my_ticket;

    my_ticket = __sync_fetch_and_add(&(lock->s.next_ticket), 1);
    while (lock->s.now_serving != my_ticket)
        __cpu_relax();

    return(0);
#elif defined(Z_PTHREAD_HAS_SPIN)
    return(pthread_spin_lock((pthread_spinlock_t *)lock));
#endif
    return(-1);
}

int z_spin_unlock (z_spinlock_t *lock) {
#if __APPLE__
    OSSpinLockUnlock((OSSpinLock *)lock);
    return(0);
#elif defined(Z_ATOMIC_GCC)
    lock->s.now_serving += 1;
    return(0);
#elif defined(Z_PTHREAD_HAS_SPIN)
    return(pthread_spin_unlock((pthread_spinlock_t *)lock));
#endif
    return(-1);
}

int z_rwlock_init (z_rwlock_t *lock) {
#if defined(Z_PTHREAD_HAS_RWLOCK)
    return(pthread_rwlock_init((pthread_rwlock_t *)lock, NULL));
#endif
    return(-1);
}

int z_rwlock_destroy (z_rwlock_t *lock) {
#if defined(Z_PTHREAD_HAS_RWLOCK)
    return(pthread_rwlock_destroy((pthread_rwlock_t *)lock));
#endif
    return(-1);
}

int z_rwlock_rdlock (z_rwlock_t *lock) {
#if defined(Z_PTHREAD_HAS_RWLOCK)
    return(pthread_rwlock_rdlock((pthread_rwlock_t *)lock));
#endif
    return(-1);
}

int z_rwlock_wrlock (z_rwlock_t *lock) {
#if defined(Z_PTHREAD_HAS_RWLOCK)
    return(pthread_rwlock_wrlock((pthread_rwlock_t *)lock));
#endif
    return(-1);
}

int z_rwlock_unlock (z_rwlock_t *lock) {
#if defined(Z_PTHREAD_HAS_RWLOCK)
    return(pthread_rwlock_unlock((pthread_rwlock_t *)lock));
#endif
    return(-1);
}

int z_mutex_init (z_mutex_t *lock) {
#if defined(Z_PTHREAD_HAS_MUTEX)
    return(pthread_mutex_init((pthread_mutex_t *)lock, NULL));
#endif
    return(-1);
}

int z_mutex_destroy (z_mutex_t *lock) {
#if defined(Z_PTHREAD_HAS_MUTEX)
    return(pthread_mutex_destroy((pthread_mutex_t *)lock));
#endif
    return(-1);
}
int z_mutex_lock (z_mutex_t *lock) {
#if defined(Z_PTHREAD_HAS_MUTEX)
    return(pthread_mutex_lock((pthread_mutex_t *)lock));
#endif
    return(-1);
}

int z_mutex_unlock (z_mutex_t *lock) {
#if defined(Z_PTHREAD_HAS_MUTEX)
    return(pthread_mutex_unlock((pthread_mutex_t *)lock));
#endif
    return(-1);
}


int z_cond_init (z_cond_t *cond) {
#if defined(Z_PTHREAD_HAS_COND)
    return(pthread_cond_init((pthread_cond_t *)cond, NULL));
#endif
    return(-1);
}

int z_cond_destroy (z_cond_t *cond) {
#if defined(Z_PTHREAD_HAS_COND)
    return(pthread_cond_destroy((pthread_cond_t *)cond));
#endif
    return(-1);
}

int z_cond_wait (z_cond_t *cond, z_mutex_t *mutex) {
#if defined(Z_PTHREAD_HAS_COND)
    return(pthread_cond_wait((pthread_cond_t *)cond, (pthread_mutex_t *)mutex));
#endif
    return(-1);
}

int z_cond_broadcast (z_cond_t *cond) {
#if defined(Z_PTHREAD_HAS_COND)
    return(pthread_cond_broadcast((pthread_cond_t *)cond));
#endif
    return(-1);
}

int z_cond_signal (z_cond_t *cond) {
#if defined(Z_PTHREAD_HAS_COND)
    return(pthread_cond_signal((pthread_cond_t *)cond));
#endif
    return(-1);
}

int z_thread_create (z_thread_t *thread,
                     void * (*func) (void *),
                     void *context)
{
#if defined(Z_PTHREAD_HAS_THREAD)
    return(pthread_create(thread, NULL, func, context));
#endif
    return(-1);
}

int z_thread_join (z_thread_t *thread) {
#if defined(Z_PTHREAD_HAS_THREAD)
    return(pthread_join(*thread, NULL));
#endif
    return(-1);
}

