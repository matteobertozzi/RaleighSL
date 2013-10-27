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


#ifndef _Z_LOCKING_H_
#define _Z_LOCKING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#if defined(Z_SYS_HAS_PTHREAD_SPIN_LOCK)
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
  #define z_spin_free(lock)
  #define z_spin_lock(lock)         OSSpinLockLock(lock)
  #define z_spin_unlock(lock)       OSSpinLockUnlock(lock)
#else
    #error "No spinlock support"
#endif

/* ============================================================================
 *  Mutex
 */
#if defined(Z_SYS_HAS_PTHREAD_MUTEX)
  #define z_mutex_t                 pthread_mutex_t
  #define z_mutex_alloc(lock)       pthread_mutex_init(lock, NULL)
  #define z_mutex_free(lock)        pthread_mutex_destroy(lock)
  #define z_mutex_lock(lock)        pthread_mutex_lock(lock)
  #define z_mutex_unlock(lock)      pthread_mutex_unlock(lock)
#else
  #error "No mutex support"
#endif

/* ============================================================================
 *  Read-Write Lock
 */
typedef struct z_rwlock {
  uint32_t state;
} z_rwlock_t;

void  z_rwlock_init             (z_rwlock_t *lock);
void  z_rwlock_read_lock        (z_rwlock_t *lock);
void  z_rwlock_read_unlock      (z_rwlock_t *lock);
void  z_rwlock_write_lock       (z_rwlock_t *lock);
void  z_rwlock_write_unlock     (z_rwlock_t *lock);
int   z_rwlock_try_read_lock    (z_rwlock_t *lock);
int   z_rwlock_try_write_lock   (z_rwlock_t *lock);

/* ============================================================================
 *  Read-Write-Commit Semaphore
 */
typedef struct z_rwcsem {
  uint32_t state;
} z_rwcsem_t;

typedef enum z_rwcsem_op {
  Z_RWCSEM_READ,
  Z_RWCSEM_WRITE,
  Z_RWCSEM_COMMIT,
  Z_RWCSEM_LOCK,
} z_rwcsem_op_t;

typedef enum z_rwcsem_state {
  Z_RWCSEM_READABLE   = 0,
  Z_RWCSEM_WRITABLE   = 1,
  Z_RWCSEM_COMMITABLE = 2,
  Z_RWCSEM_LOCKABLE   = 3,
} z_rwcsem_state_t;

int   z_rwcsem_init                 (z_rwcsem_t *lock);

int   z_rwcsem_try_acquire_read     (z_rwcsem_t *lock);
int   z_rwcsem_release_read         (z_rwcsem_t *lock);

int   z_rwcsem_try_acquire_write    (z_rwcsem_t *lock);
int   z_rwcsem_release_write        (z_rwcsem_t *lock);

int   z_rwcsem_has_commit_flag      (z_rwcsem_t *lock);
void  z_rwcsem_set_commit_flag      (z_rwcsem_t *lock);
int   z_rwcsem_try_acquire_commit   (z_rwcsem_t *lock);
int   z_rwcsem_release_commit       (z_rwcsem_t *lock);

int   z_rwcsem_has_lock_flag        (z_rwcsem_t *lock);
void  z_rwcsem_set_lock_flag        (z_rwcsem_t *lock);
int   z_rwcsem_try_acquire_lock     (z_rwcsem_t *lock);
int   z_rwcsem_release_lock         (z_rwcsem_t *lock);

int   z_rwcsem_try_acquire  (z_rwcsem_t *lock, z_rwcsem_op_t operation);
int   z_rwcsem_release      (z_rwcsem_t *lock, z_rwcsem_op_t operation);

/* ============================================================================
 *  With Lock
 */
#define z_lock(lock, type, __code__)                \
  do {                                              \
    type ## _lock(lock);                            \
    do __code__ while (0);                          \
    type ## _unlock(lock);                          \
  } while (0)

#define z_read_lock(lock, type, __code__)           \
  do {                                              \
    type ## _read_lock(lock);                       \
    do __code__ while (0);                          \
    type ## _read_unlock(lock);                     \
  } while (0)

#define z_write_lock(lock, type, __code__)          \
  do {                                              \
    type ## _write_lock(lock);                      \
    do __code__ while (0);                          \
    type ## _write_unlock(lock);                    \
  } while (0)

#define z_try_write_lock(lock, type, __code__)      \
  if (type ## _try_write_lock(lock)) {              \
    do __code__ while (0);                          \
    type ## _write_unlock(lock);                    \
  }

#define z_try_read_lock(lock, type, __code__)       \
  if (type ## _try_read_lock(lock)) {               \
    do __code__ while (0);                          \
    type ## _read_unlock(lock);                     \
  }

__Z_END_DECLS__

#endif /* _Z_LOCKING_H_ */
