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

#ifndef _Z_TIME_H_
#define _Z_TIME_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/debug.h>

#if defined(Z_SYS_HAS_CLOCK_GETTIME)
  #include <time.h>
#endif

Z_TYPEDEF_STRUCT(z_timer)

#define Z_TIME_USEC(x)            ((x) * 1)
#define Z_TIME_MSEC(x)            ((x) * 1000U)
#define Z_TIME_SEC(x)             ((x) * 1000000)

#define z_timeval_to_micros(x)    ((x)->tv_sec * 1000000U + (x)->tv_usec)
#define z_usec_to_sec(t)          ((t) / 1000000.0)

struct z_timer {
  uint64_t start;
  uint64_t end;
};

#define Z_TRACE_TIME(name, code)                                    \
  do {                                                              \
    z_timer_t __ ## name ## _timer__;                               \
    z_timer_start(&__ ## name ## _timer__);                         \
    do code while (0);                                              \
    z_timer_stop(&__ ## name ## _timer__);                          \
    Z_LOG_TRACE("TIMING %s executed in %.5fsec", # name,            \
                 z_timer_secs(&__ ## name ## _timer__));            \
  } while (0)

uint32_t z_time_secs    (void);
uint64_t z_time_millis  (void);
uint64_t z_time_micros  (void);
uint64_t z_time_nanos   (void);

void     z_time_usleep  (uint64_t usec);

#define z_timer_start(t)        (t)->start = z_time_micros()
#define z_timer_stop(t)         (t)->end = z_time_micros()
#define z_timer_reset(t)        (t)->start = (t)->end
#define z_timer_micros(t)       ((t)->end - (t)->start)
#define z_timer_millis(t)       (z_timer_micros(t) / 1000)
#define z_timer_secs(t)         (z_timer_micros(t) / 1000000.0)

__Z_END_DECLS__

#endif /* _Z_TIME_H_ */
