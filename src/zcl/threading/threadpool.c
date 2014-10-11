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

#include <zcl/threadpool.h>
#include <zcl/debug.h>

int z_thread_pool_open (z_thread_pool_t *self, unsigned int nthreads) {
  if (z_mutex_alloc(&(self->mutex))) {
    Z_LOG_FATAL("unable to initialize the thread-pool mutex.");
    return(1);
  }

  if (z_wait_cond_alloc(&(self->task_ready))) {
    Z_LOG_FATAL("unable to initialize the thread-pool wait condition 0.");
    z_mutex_free(&(self->mutex));
    return(2);
  }

  if (z_wait_cond_alloc(&(self->no_active_threads))) {
    Z_LOG_FATAL("unable to initialize the thread-pool wait condition 1.");
    z_wait_cond_free(&(self->task_ready));
    z_mutex_free(&(self->mutex));
    return(3);
  }

  self->waiting_threads = 0;
  self->size = nthreads & 0xffff;
  self->is_running = 1;
  self->balancer = 0;
  return(0);
}

void z_thread_pool_wait (z_thread_pool_t *self) {
  z_mutex_lock(&(self->mutex));
  if (self->waiting_threads < self->size) {
    z_wait_cond_wait(&(self->no_active_threads), &(self->mutex), 0);
  }
  z_mutex_unlock(&(self->mutex));
}

void z_thread_pool_stop (z_thread_pool_t *self) {
  z_mutex_lock(&(self->mutex));
  if (self->is_running) {
    self->is_running = 0;
    z_wait_cond_broadcast(&(self->task_ready));
  }
  z_mutex_unlock(&(self->mutex));
}

void z_thread_pool_close (z_thread_pool_t *self) {
  z_thread_pool_stop(self);
  z_wait_cond_free(&(self->no_active_threads));
  z_wait_cond_free(&(self->task_ready));
  z_mutex_free(&(self->mutex));
}

void z_thread_pool_worker_close (z_thread_pool_t *self) {
  z_mutex_lock(&(self->mutex));
  if (++self->waiting_threads == self->size) {
    z_wait_cond_broadcast(&(self->no_active_threads));
  }
  z_mutex_unlock(&(self->mutex));
}

int z_thread_pool_worker_wait_on (z_thread_pool_t *self,
                                  z_wait_cond_t *task_ready,
                                  unsigned int usec)
{
  int is_running;
  z_mutex_lock(&(self->mutex));
  if (Z_UNLIKELY(++self->waiting_threads == self->size)) {
    z_wait_cond_broadcast(&(self->no_active_threads));
  }
  z_wait_cond_wait(task_ready, &(self->mutex), usec);
  self->waiting_threads -= 1;
  is_running = self->is_running;
  z_mutex_unlock(&(self->mutex));
  return(is_running);
}
