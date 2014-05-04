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

#include <zcl/thread.h>
#include <zcl/system.h>
#include <zcl/debug.h>

/* ============================================================================
 *  Threads
 */
#define __THREAD_MIN_STACK_SIZE          (8192)

int z_thread_start (z_thread_t *tid, z_thread_func_t func, void *args) {
#if defined(Z_SYS_HAS_PTHREAD_THREAD)
  pthread_attr_t attr;
  size_t stacksize;
  int res;

  pthread_attr_init(&attr);

  /*
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  */

  pthread_attr_getstacksize(&attr, &stacksize);
  if (stacksize < __THREAD_MIN_STACK_SIZE) {
    pthread_attr_setstacksize(&attr, stacksize);
  }

  res = pthread_create(tid, &attr, func, args);
  pthread_attr_destroy(&attr);
  return(res);
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

int z_thread_get_core (z_thread_t *thread, int *core) {
#if defined(Z_SYS_HAS_PTHREAD_AFFINITY)
  cpu_set_t cpuset;
  int i;
  CPU_ZERO(&cpuset);
  if (pthread_getaffinity_np(*thread, sizeof(cpu_set_t), &cpuset)) {
    Z_LOG_WARN("unable to identify thread %p core", thread);
    return(-1);
  }

  for (i = 0; i < CPU_SETSIZE; i++) {
    if (CPU_ISSET(i, &cpuset)) {
      return(i);
    }
  }
  Z_LOG_WARN("unable to identify thread %p core", thread);
  return(-2);
#else
  Z_LOG_WARN("unable to identify thread %p core", thread);
  return(-1);
#endif
}
