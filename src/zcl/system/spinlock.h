/*
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


#ifndef _Z_SPINLOCK_H_
#define _Z_SPINLOCK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define Z_USE_TICKET_SPIN_LOCK
#if defined(Z_SYS_HAS_PTHREAD)
  #include <pthread.h>
#endif

#if defined(Z_SYS_HAS_OS_SPIN_LOCK)
  #include <libkern/OSAtomic.h>
#endif

#if defined(Z_USE_TICKET_SPIN_LOCK)
  #include <zcl/ticket.h>
#endif

/* ============================================================================
 *  Spin Lock
 */
#if defined(Z_USE_TICKET_SPIN_LOCK)
  #define z_spinlock_t              z_ticket_t
  #define z_spin_alloc(lock)        z_ticket_init(lock)
  #define z_spin_free(lock)
  #define z_spin_lock(lock)         z_ticket_acquire(lock)
  #define z_spin_unlock(lock)       z_ticket_release(lock)
#elif defined(Z_SYS_HAS_PTHREAD_SPIN_LOCK)
  #define z_spinlock_t              pthread_spinlock_t
  #define z_spin_alloc(lock)        pthread_spin_init(lock, 0)
  #define z_spin_free(lock)         pthread_spin_destroy(lock)
  #define z_spin_lock(lock)         pthread_spin_lock(lock)
  #define z_spin_unlock(lock)       pthread_spin_unlock(lock)
#elif defined(Z_SYS_HAS_OS_SPIN_LOCK)
  #define z_spinlock_t              OSSpinLock
  #define z_spin_alloc(lock)        (*(lock) = OS_SPINLOCK_INIT)
  #define z_spin_free(lock)         (void)lock;
  #define z_spin_lock(lock)         OSSpinLockLock(lock)
  #define z_spin_unlock(lock)       OSSpinLockUnlock(lock)
#else
    #error "No spinlock support"
#endif

__Z_END_DECLS__

#endif /* _Z_SPINLOCK_H_ */
