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
int z_thread_start (z_thread_t *tid, z_thread_func func, void *args) {
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

  Z_LOG_WARN("unable to bind thread %p to core %d", core);
  return(-1);
#endif
}
