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

#ifndef _Z_SEMAPHORE_
#define _Z_SEMAPHORE_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/locking.h>
#include <zcl/threading.h>

#define Z_SEMAPHORE(x)             Z_CAST(z_semaphore_t, x)

#define Z_LATCH                    Z_SEMAPHORE
#define z_latch_t                  z_semaphore_t

Z_TYPEDEF_STRUCT(z_semaphore)

struct z_semaphore {
  z_wait_cond_t wcond;
  z_mutex_t mutex;
  int available;
};

int  z_semaphore_open              (z_semaphore_t *self, int available);
void z_semaphore_close             (z_semaphore_t *self);
void z_semaphore_acquire           (z_semaphore_t *self, int n);
int  z_semaphore_try_acquire       (z_semaphore_t *self, int n);
int  z_semaphore_try_acquire_timed (z_semaphore_t *self, int n, unsigned int msec);
void z_semaphore_release           (z_semaphore_t *self, int n);

#define z_latch_open(self)                  z_semaphore_open(self, 0)
#define z_latch_close(self)                 z_semaphore_close(self)
#define z_latch_release(self)               z_semaphore_release(self, 1)
#define z_latch_wait(self, n)               z_semaphore_acquire(self, n)
#define z_latch_timed_wait(self, n, msec)   z_semaphore_try_acquire_timed(self, n)

__Z_END_DECLS__

#endif /* _Z_SEMAPHORE_ */

