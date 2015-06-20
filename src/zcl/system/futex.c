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

#include <zcl/config.h>
#if defined(Z_SYS_HAS_FUTEX)

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include <zcl/system.h>
#include <zcl/atomic.h>
#include <zcl/mutex.h>
#include <zcl/waitcond.h>

#define __LOCK_SPINS      (100)
#define __UNLOCK_SPINS    (200)

int z_mutex_alloc (z_mutex_t *m) {
  m->u = 0;
  return 0;
}

int z_mutex_free(z_mutex_t *m) {
  return 0;
}

int z_mutex_lock(z_mutex_t *m) {
  int i;

  /* Try to grab lock */
  for (i = 0; i < __LOCK_SPINS; i++) {
    if (!__sync_lock_test_and_set(&(m->b.locked), 1))
      return 0;
    z_system_cpu_relax();
  }

  /* Have to sleep */
  while (__sync_lock_test_and_set(&(m->u), 257) & 1) {
    syscall(__NR_futex, m, FUTEX_WAIT_PRIVATE, 257, NULL, NULL, 0);
  }
  return 0;
}

int z_mutex_unlock(z_mutex_t *m) {
  int i;

  /* Locked and not contended */
  if ((m->u == 1) && (__sync_val_compare_and_swap(&(m->u), 1, 0) == 1))
    return 0;

  /* Unlock */
  m->b.locked = 0;
  __sync_synchronize();

  /* Spin and hope someone takes the lock */
  for (i = 0; i < __UNLOCK_SPINS; i++) {
    if (m->b.locked)
      return 0;

    z_system_cpu_relax();
  }

  /* We need to wake someone up */
  m->b.contended = 0;

  syscall(__NR_futex, m, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
  return 0;
}

int z_mutex_try_lock(z_mutex_t *m) {
  return(__sync_lock_test_and_set(&(m->b.locked), 1));
}


int z_wait_cond_alloc(z_wait_cond_t *c) {
  c->m = NULL;
  c->seq = 0;
  return 0;
}

int z_wait_cond_free(z_wait_cond_t *c) {
  return 0;
}

int z_wait_cond_signal(z_wait_cond_t *c) {
  /* We are waking someone up */
  __sync_add_and_fetch(&(c->seq), 1);

  /* Wake up a thread */
  syscall(__NR_futex, &(c->seq), FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
  return 0;
}

int z_wait_cond_broadcast(z_wait_cond_t *c) {
  z_mutex_t *m = c->m;

  /* No z_mutex_t means that there are no waiters */
  if (!m)
    return 0;

  /* We are waking everyone up */
  __sync_add_and_fetch(&(c->seq), 1);

  /* Wake one thread, and requeue the rest on the z_mutex_t */
  syscall(__NR_futex, &(c->seq), FUTEX_REQUEUE_PRIVATE, 1, (void *)0xffffffff, m, 0);
  return 0;
}

int z_wait_cond_wait(z_wait_cond_t *c, z_mutex_t *m, unsigned long usec) {
  struct timespec timeout;
  int seq = c->seq;

  if (Z_UNLIKELY(c->m != m)) {
    /* Atomically set z_mutex_t inside cv */
    if (__sync_val_compare_and_swap(&c->m, NULL, m) != NULL) {
      fprintf(stderr, "z_wait_cond_wait(): wrong mutex swap\n");
      return 1;
    }
  }

  z_mutex_unlock(m);

  timeout.tv_sec  = usec / 1000000;
  timeout.tv_nsec = (usec % 1000000) * 1000;
  syscall(__NR_futex, &(c->seq), FUTEX_WAIT_PRIVATE, seq, &timeout, NULL, 0);

  while (__sync_lock_test_and_set(&(m->b.locked), 257) & 1) {
    syscall(__NR_futex, m, FUTEX_WAIT_PRIVATE, 257, &timeout, NULL, 0);
  }
  return 0;
}

#endif /* Z_SYS_HAS_FUTEX */
