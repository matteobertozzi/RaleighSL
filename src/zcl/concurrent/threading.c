/*
 *   Copyright 2011-2013 Matteo Bertozzi
 *
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

#if defined(Z_SYS_HAS_PTHREAD_WAIT_COND)
int z_wait_cond_alloc (z_wait_cond_t *self) {
    if (pthread_mutex_init(&(self->wlock), NULL))
        return(-1);

    if (pthread_cond_init(&(self->wcond), NULL)) {
        pthread_mutex_destroy(&(self->wlock));
        return(-2);
    }

    return(0);
}

void z_wait_cond_free  (z_wait_cond_t *self) {
    pthread_mutex_destroy(&(self->wlock));
    pthread_cond_destroy(&(self->wcond));
}

int z_wait_cond_wait (z_wait_cond_t *self, unsigned long msec) {
    pthread_mutex_lock(&(self->wlock));
    if (msec > 0) {
        pthread_cond_wait(&(self->wcond), &(self->wlock));
    } else {
        struct timespec timeout;
        timeout.tv_sec  = msec / 1000;
        timeout.tv_nsec = (msec % 1000) * 1000;
        pthread_cond_timedwait(&(self->wcond), &(self->wlock), &timeout);
    }
    pthread_mutex_unlock(&(self->wlock));
    return(0);
}

int z_wait_cond_signal (z_wait_cond_t *self) {
    return(pthread_cond_signal(&(self->wcond)));
}

int z_wait_cond_broadcast (z_wait_cond_t *self) {
    return(pthread_cond_broadcast(&(self->wcond)));
}
#else
    #error "No wait condition"
#endif

int z_thread_bind_to_core (z_thread_t *thread, unsigned int core) {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   if (core >= num_cores)
      return(1);

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core, &cpuset);

   return pthread_setaffinity_np(*thread, sizeof(cpu_set_t), &cpuset);
}