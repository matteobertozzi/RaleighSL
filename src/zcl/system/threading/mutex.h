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

#if defined(Z_SYS_HAS_PTHREAD)
  #include <pthread.h>
#endif

#if defined(Z_SYS_HAS_PTHREAD_MUTEX)
  #define z_mutex_t                 pthread_mutex_t

  #define z_mutex_shmem_init(lock) ({                                        \
    int __res;                                                               \
    pthread_mutexattr_t __init_attrmutex;                                    \
    pthread_mutexattr_init(&__init_attrmutex);                               \
    pthread_mutexattr_setpshared(&__init_attrmutex, PTHREAD_PROCESS_SHARED); \
    __res = pthread_mutex_init(lock, &__init_attrmutex);                     \
    pthread_mutexattr_destroy(&__init_attrmutex);                            \
    __res;                                                                   \
  })

  #define z_mutex_init(lock)        pthread_mutex_init(lock, NULL)
  #define z_mutex_destroy(lock)     pthread_mutex_destroy(lock)
  #define z_mutex_try_lock(lock)    !pthread_mutex_trylock(lock)
  #define z_mutex_lock(lock)        pthread_mutex_lock(lock)
  #define z_mutex_unlock(lock)      pthread_mutex_unlock(lock)
#else
  #error "No mutex support"
#endif


__Z_END_DECLS__

#endif /* !_Z_MUTEX_H_ */