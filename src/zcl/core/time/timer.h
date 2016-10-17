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

#ifndef _Z_TIMER_H_
#define _Z_TIMER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/time.h>

typedef struct z_timer {
  uint64_t start;
  uint64_t end;
} z_timer_t;

#define z_timer_start(t)            (t)->start = z_time_monotonic()
#define z_timer_stop(t)             (t)->end = z_time_monotonic()
#define z_timer_reset(t)            (t)->start = (t)->end
#define z_timer_nanos(t)            ((t)->end - (t)->start)
#define z_timer_secs(t)             Z_TIME_NANOS_TO_SECONDS_F(z_timer_nanos(t))
#define z_timer_elapsed_nanos(t)    (z_time_monotonic() - (t)->start)
#define z_timer_elapsed_secs(t)     Z_TIME_NANOS_TO_SECONDS_F(z_timer_elapsed_nanos(t))

__Z_END_DECLS__

#endif /* !_Z_TIMER_H_ */
