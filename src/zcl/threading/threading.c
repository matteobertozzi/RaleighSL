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

#include <unistd.h>

#include <zcl/threading.h>
#include <zcl/system.h>
#include <zcl/debug.h>

/* ============================================================================
 *  Wait condition
 */
#if defined(Z_SYS_HAS_PTHREAD_WAIT_COND)
void z_wait_cond_wait (z_wait_cond_t *wcond,
                       z_mutex_t *mutex,
                       unsigned long msec)
{
  if (msec == 0) {
    pthread_cond_wait(wcond, mutex);
  } else {
    struct timespec timeout;
    timeout.tv_sec  = msec / 1000;
    timeout.tv_nsec = (msec % 1000) * 1000;
    pthread_cond_timedwait(wcond, mutex, &timeout);
  }
}
#else
  #error "No wait condition"
#endif

/* ============================================================================
 *  Threads
 */
int z_thread_start (z_thread_t *tid, z_thread_func_t func, void *args) {
#if defined(Z_SYS_HAS_PTHREAD_THREAD)
#if 0
  pthread_attr_t attr;
  int res;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  res = pthread_create(tid, &attr, func, args);
  pthread_attr_destroy(&attr);
  return(res);
#else
  return(pthread_create(tid, NULL, func, args));
#endif
#else
  #error "No thread support"
#endif
}

int z_thread_bind_to_core (z_thread_t *thread, int core) {
  Z_ASSERT(core >= 0, "core must be greater than 0, got %d", core);
#if defined(Z_SYS_HAS_PTHREAD_AFFINITY)
  unsigned int ncores = z_system_processors();
  if (Z_UNLIKELY(core >= ncores))
    return(core % ncores);

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  return pthread_setaffinity_np(*thread, sizeof(cpu_set_t), &cpuset);
#else
  /*
   * https://developer.apple.com/library/mac/releasenotes/Performance/RN-AffinityAPI/index.html
   */

  Z_LOG_WARN("unable to bind thread %p to core %d", thread, core);
  return(-1);
#endif
}

/* ============================================================================
 *  Thread Pool/Task Queue Utils
 */
int z_thread_pool_open (z_thread_pool_t *self, unsigned int nthreads) {
  if (z_mutex_alloc(&(self->mutex))) {
    Z_LOG_FATAL("unable to initialize the thread-pool mutex.");
    return(1);
  }

  /* Initialize the global task queue */
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
  self->total_threads = nthreads & 0xffff;
  self->is_running = 1;
  return(0);
}

void z_thread_pool_wait (z_thread_pool_t *self) {
  z_mutex_lock(&(self->mutex));
  if (self->waiting_threads < self->total_threads) {
    z_wait_cond_wait(&(self->no_active_threads), &(self->mutex), 0);
  }
  z_mutex_unlock(&(self->mutex));
}

void z_thread_pool_stop (z_thread_pool_t *self) {
  /* Send close notification and wait */
  z_mutex_lock(&(self->mutex));
  if (self->is_running) {
    self->is_running = 0;
    z_wait_cond_broadcast(&(self->task_ready));
    if (self->waiting_threads < self->total_threads) {
      z_wait_cond_wait(&(self->no_active_threads), &(self->mutex), 0);
    }
  }
  z_mutex_unlock(&(self->mutex));
}

void z_thread_pool_close (z_thread_pool_t *self) {
  z_thread_pool_stop(self);
  z_wait_cond_free(&(self->no_active_threads));
  z_wait_cond_free(&(self->task_ready));
  z_mutex_free(&(self->mutex));
}

void z_thread_pool_ctx_close (z_thread_pool_t *self) {
  z_mutex_lock(&(self->mutex));
  if (++self->waiting_threads == self->total_threads) {
    z_wait_cond_broadcast(&(self->no_active_threads));
  }
  z_mutex_unlock(&(self->mutex));
}

void *z_thread_pool_ctx_fetch (z_thread_pool_t *self,
                               z_thread_func_t func,
                               void *udata,
                               int *is_running)
{
  void *task;

  z_mutex_lock(&(self->mutex));
  *is_running = self->is_running;
  if (Z_UNLIKELY(!self->is_running)) {
    z_mutex_unlock(&(self->mutex));
    return(NULL);
  }

  task = func(udata);
  if (Z_UNLIKELY(task == NULL)) {
    if (++self->waiting_threads == self->total_threads)
      z_wait_cond_broadcast(&(self->no_active_threads));
    z_wait_cond_wait(&(self->task_ready), &(self->mutex), 0);

    *is_running = self->is_running;
    task = func(udata);
  }
  z_mutex_unlock(&(self->mutex));
  return(task);
}