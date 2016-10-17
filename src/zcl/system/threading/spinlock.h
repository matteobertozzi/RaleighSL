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

#if defined(Z_SYS_HAS_PTHREAD_SPIN_LOCK)
  #include <pthread.h>
#elif defined(Z_SYS_HAS_OS_UNFAIR_LOCK)
  #include <os/lock.h>
#elif defined(Z_SYS_HAS_OS_SPIN_LOCK)
  #include <libkern/OSAtomic.h>
#endif

#if defined(Z_SYS_HAS_PTHREAD_SPIN_LOCK)
  #define z_spinlock_t              pthread_spinlock_t
  #define z_spin_alloc(lock)        pthread_spin_init(lock, 0)
  #define z_spin_free(lock)         pthread_spin_destroy(lock)
  #define z_spin_try_lock(lock)     !pthread_spin_trylock(lock)
  #define z_spin_lock(lock)         pthread_spin_lock(lock)
  #define z_spin_unlock(lock)       pthread_spin_unlock(lock)
#elif defined(Z_SYS_HAS_OS_UNFAIR_LOCK)
  #define z_spinlock_t              os_unfair_lock_t
  #define z_spin_alloc(lock)        *lock = &OS_UNFAIR_LOCK_INIT
  #define z_spin_free(lock)         (void)lock;
  #define z_spin_try_lock(lock)     os_unfair_lock_trylock(*(lock))
  #define z_spin_lock(lock)         os_unfair_lock_lock(*(lock))
  #define z_spin_unlock(lock)       os_unfair_lock_unlock(*(lock))
#elif defined(Z_SYS_HAS_OS_SPIN_LOCK)
  #define z_spinlock_t              OSSpinLock
  #define z_spin_alloc(lock)        (*(lock) = OS_SPINLOCK_INIT)
  #define z_spin_free(lock)         (void)lock;
  #define z_spin_try_lock(lock)     OSSpinLockTry(lock)
  #define z_spin_lock(lock)         OSSpinLockLock(lock)
  #define z_spin_unlock(lock)       OSSpinLockUnlock(lock)
#else
  #error "No spinlock support"
#endif

__Z_END_DECLS__

#endif /* !_Z_SPINLOCK_H_ */
