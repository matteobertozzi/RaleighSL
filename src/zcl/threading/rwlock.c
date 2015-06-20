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

#include <zcl/rwlock.h>
#include <zcl/atomic.h>
#include <zcl/thread.h>

/* ============================================================================
 *  Read-Write lock
 */
#define Z_RWLOCK_NUM_READERS_MASK         (0x7fffffff)
#define Z_RWLOCK_WRITE_FLAG               (1 << 31)

void z_rwlock_init (z_rwlock_t *lock) {
  lock->state = 0;
}

void z_rwlock_read_lock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval & Z_RWLOCK_NUM_READERS_MASK;   /* no wrlock expval */
    newval = expval + 1;                           /* Add me as reader */
  });
}

void z_rwlock_read_unlock (z_rwlock_t *lock) {
#if 0
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval;       /* I expect a write lock and other readers */
    newval = expval - 1;   /* Drop me as reader */
  });
#else
  z_atomic_sub_and_fetch(&(lock->state), 1);
#endif
}

void z_rwlock_write_lock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    expval = curval & Z_RWLOCK_NUM_READERS_MASK;  /* I expect some 0+ readers */
    newval = Z_RWLOCK_WRITE_FLAG | expval;        /* I want to lock the other writers */
  });

  /* Wait pending reads */
  while ((lock->state & Z_RWLOCK_NUM_READERS_MASK) > 0)
    z_thread_yield();
}

void z_rwlock_write_unlock (z_rwlock_t *lock) {
  z_atomic_synchronize();
  lock->state = 0;
}

int z_rwlock_try_read_lock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (curval & Z_RWLOCK_WRITE_FLAG)
      return(0);

    expval = curval & Z_RWLOCK_NUM_READERS_MASK;  /* I expect some 0+ readers */
    newval = expval + 1;                          /* Add me as reader */
  });
  return(1);
}

int z_rwlock_try_write_lock (z_rwlock_t *lock) {
  uint32_t curval, expval, newval;
  z_atomic_cas_loop(&(lock->state), curval, expval, newval, {
    if (curval & Z_RWLOCK_WRITE_FLAG)
      return(0);

    expval = curval & Z_RWLOCK_NUM_READERS_MASK;      /* I expect some 0+ readers */
    newval = Z_RWLOCK_WRITE_FLAG | expval;  /* I want to lock the other writers */
  });

  /* Wait pending reads */
  while ((lock->state & Z_RWLOCK_NUM_READERS_MASK) > 0)
    z_thread_yield();
  return(1);
}
