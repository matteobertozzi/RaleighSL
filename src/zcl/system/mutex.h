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

#ifndef _Z_MUTEX_H_
#define _Z_MUTEX_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define Z_USE_TICKET_SPIN_LOCK
#if defined(Z_SYS_HAS_PTHREAD)
  #include <pthread.h>
#endif

#if defined(Z_SYS_HAS_FUTEX)
  typedef union z_mutex {
    uint32_t u;
    struct {
      uint16_t locked;
      uint16_t contended;
    } b;
  } z_mutex_t;

  int z_mutex_alloc (z_mutex_t *self);
  int z_mutex_free(z_mutex_t *m);
  int z_mutex_lock(z_mutex_t *m);
  int z_mutex_unlock(z_mutex_t *m);
  int z_mutex_try_lock(z_mutex_t *m);
#elif defined(Z_SYS_HAS_PTHREAD_MUTEX)
  #define z_mutex_t                 pthread_mutex_t
  #define z_mutex_alloc(lock)       pthread_mutex_init(lock, NULL)
  #define z_mutex_free(lock)        pthread_mutex_destroy(lock)
  #define z_mutex_lock(lock)        pthread_mutex_lock(lock)
  #define z_mutex_unlock(lock)      pthread_mutex_unlock(lock)
#else
  #error "No mutex support"
#endif

__Z_END_DECLS__

#endif /* !_Z_MUTEX_H_ */
