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

#ifndef _Z_WAIT_COND_H_
#define _Z_WAIT_COND_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/mutex.h>

#if defined(Z_SYS_HAS_PTHREAD)
  #include <pthread.h>
#endif

#if defined(Z_SYS_HAS_PTHREAD_WAIT_COND)
  typedef pthread_cond_t z_wait_cond_t;

  #define z_wait_cond_alloc(wcond)        pthread_cond_init(wcond, NULL)
  #define z_wait_cond_free(wcond)         pthread_cond_destroy(wcond)
  #define z_wait_cond_signal(wcond)       pthread_cond_signal(wcond)
  #define z_wait_cond_broadcast(wcond)    pthread_cond_broadcast(wcond)
  void    z_wait_cond_wait               (z_wait_cond_t *wcond,
                                          z_mutex_t *mutex,
                                          unsigned long usec);
#else
  #error "No wait condition support"
#endif

__Z_END_DECLS__

#endif /* _Z_WAIT_COND_H_ */
