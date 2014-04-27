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

#include <zcl/waitcond.h>
#include <zcl/time.h>

/* ============================================================================
 *  Wait condition
 */
#if defined(Z_SYS_HAS_PTHREAD_WAIT_COND)
void z_wait_cond_wait (z_wait_cond_t *wcond,
                       z_mutex_t *mutex,
                       unsigned long usec)
{
  if (usec == 0) {
    pthread_cond_wait(wcond, mutex);
  } else {
    struct timespec timeout;
    usec = z_time_micros() + usec;
    timeout.tv_sec  = usec / 1000000;
    timeout.tv_nsec = (usec % 1000000) * 1000;
    pthread_cond_timedwait(wcond, mutex, &timeout);
  }
}
#else
  #error "No wait condition"
#endif
