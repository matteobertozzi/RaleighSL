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

#ifndef _Z_THREADING_H_
#define _Z_THREADING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#if defined(Z_SYS_HAS_PTHREAD)
  #include <pthread.h>
#endif

#include <zcl/locking.h>

Z_TYPEDEF_STRUCT(z_thread_pool)

typedef void *(*z_thread_func_t) (void *);

#if defined(Z_SYS_HAS_PTHREAD_WAIT_COND)
  typedef pthread_cond_t z_wait_cond_t;

  #define z_wait_cond_alloc(wcond)        pthread_cond_init(wcond, NULL)
  #define z_wait_cond_free(wcond)         pthread_cond_destroy(wcond)
  #define z_wait_cond_signal(wcond)       pthread_cond_signal(wcond)
  #define z_wait_cond_broadcast(wcond)    pthread_cond_broadcast(wcond)
  void    z_wait_cond_wait               (z_wait_cond_t *wcond,
                                          z_mutex_t *mutex,
                                          unsigned long msec);
#else
  #error "No wait condition support"
#endif

#if defined(Z_SYS_HAS_PTHREAD)
  #define z_thread_t                       pthread_t
  #define z_thread_join(tid)               pthread_join(*(tid), NULL)
  #define z_thread_self(tid)               *(tid) = pthread_self()

  #if defined(Z_SYS_HAS_PTHREAD_YIELD)
    #define z_thread_yield()                 pthread_yield()
  #elif defined(Z_SYS_HAS_PTHREAD_YIELD_NP)
    #define z_thread_yield()                 pthread_yield_np()
  #else
    /* https://developer.apple.com/library/mac/releasenotes/Performance/RN-AffinityAPI/index.html */
    #error "No thread yield supported"
  #endif
#else
  #error "No thread support"
#endif

struct z_thread_pool {
  z_mutex_t     mutex;
  z_wait_cond_t task_ready;
  z_wait_cond_t no_active_threads;
  uint16_t  waiting_threads;
  uint16_t  total_threads;
  uint8_t   is_running;
};

int   z_thread_start          (z_thread_t *tid, z_thread_func_t func, void *args);
int   z_thread_bind_to_core   (z_thread_t *thread, int core);

int   z_thread_pool_open      (z_thread_pool_t *self, unsigned int nthreads);
void  z_thread_pool_wait      (z_thread_pool_t *self);
void  z_thread_pool_stop      (z_thread_pool_t *self);
void  z_thread_pool_close     (z_thread_pool_t *self);
void  z_thread_pool_ctx_close (z_thread_pool_t *self);
void *z_thread_pool_ctx_fetch (z_thread_pool_t *self,
                               z_thread_func_t func,
                               void *udata,
                               int *is_running);

__Z_END_DECLS__

#endif /* _Z_THREADING_H_ */
