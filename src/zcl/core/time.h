/*
 *   Copyright 2011 Matteo Bertozzi
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

#ifndef _Z_TIME_H_
#define _Z_TIME_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <stdint.h>
#include <time.h>

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_timer)

struct z_timer {
    uint64_t micros[2];
    clock_t  clock[2];
};

uint64_t    z_time_millis       (void);
uint64_t    z_time_micros       (void);
uint64_t    z_time_nanos        (void);
#define     z_time_now          z_time_nanos

void        z_timer_start       (z_timer_t *timer);
void        z_timer_stop        (z_timer_t *timer);
float       z_timer_clock       (const z_timer_t *timer);
float       z_timer_micros      (const z_timer_t *timer);
float       z_timer_secs        (const z_timer_t *timer);

__Z_END_DECLS__

#endif /* !_Z_TIME_H_ */

