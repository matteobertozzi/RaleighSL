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

#ifndef _Z_THREAD_POOL_H_
#define _Z_THREAD_POOL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/waitcond.h>
#include <zcl/system.h>
#include <zcl/thread.h>
#include <zcl/macros.h>
#include <zcl/mutex.h>

Z_TYPEDEF_STRUCT(z_thread_pool)

struct z_thread_pool {
  z_mutex_t     mutex;
  z_wait_cond_t task_ready;
  z_wait_cond_t no_active_threads;
  uint16_t      waiting_threads;
  uint16_t      total_threads;
  uint8_t       is_running;
  uint8_t       pad[3];
};

int   z_thread_pool_open          (z_thread_pool_t *self,
                                   unsigned int nthreads);
void  z_thread_pool_wait          (z_thread_pool_t *self);
void  z_thread_pool_stop          (z_thread_pool_t *self);
void  z_thread_pool_close         (z_thread_pool_t *self);

void  z_thread_pool_worker_close  (z_thread_pool_t *self);
int   z_thread_pool_worker_wait   (z_thread_pool_t *self);

__Z_END_DECLS__

#endif /* _Z_THREAD_POOL_H_ */
