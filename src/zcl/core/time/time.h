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

uint64_t z_time_nanos     (void);
uint64_t z_time_micros    (void);
uint64_t z_time_millis    (void);

uint64_t z_time_monotonic (void);

void     z_time_usleep    (uint64_t usec);

#define Z_TIME_NANOS_TO_MICROS(x)           ((x) / 1000ul)
#define Z_TIME_NANOS_TO_MILLIS(x)           ((x) / 1000000ul)
#define Z_TIME_NANOS_TO_SECONDS(x)          ((x) / 1000000000ul)
#define Z_TIME_NANOS_TO_MINUTES(x)          ((x) / 60000000000ul)

#define Z_TIME_NANOS_TO_MICROS_F(x)         ((x) / 1000.0)
#define Z_TIME_NANOS_TO_MILLIS_F(x)         ((x) / 1000000.0)
#define Z_TIME_NANOS_TO_SECONDS_F(x)        ((x) / 1000000000.0)
#define Z_TIME_NANOS_TO_MINUTES_F(x)        ((x) / 60000000000.0)

#define Z_TIME_MICROS_TO_NANOS(x)           ((x) * 1000ul)
#define Z_TIME_MICROS_TO_MILLIS(x)          ((x) / 1000ul)
#define Z_TIME_MICROS_TO_SECONDS(x)         ((x) / 1000000ul)
#define Z_TIME_MICROS_TO_MINUTES(x)         ((x) / 60000000ul)

#define Z_TIME_MICROS_TO_NANOS_F(x)         ((x) * 1000.0)
#define Z_TIME_MICROS_TO_MILLIS_F(x)        ((x) / 1000.0)
#define Z_TIME_MICROS_TO_SECONDS_F(x)       ((x) / 1000000.0)
#define Z_TIME_MICROS_TO_MINUTES_F(x)       ((x) / 60000000.0)

#define Z_TIME_MILLIS_TO_NANOS(x)           ((x) * 1000000ul)
#define Z_TIME_MILLIS_TO_MICROS(x)          ((x) * 1000ul)
#define Z_TIME_MILLIS_TO_SECONDS(x)         ((x) / 1000ul)
#define Z_TIME_MILLIS_TO_MINUTES(x)         ((x) / 60000ul)

#define Z_TIME_MILLIS_TO_NANOS_F(x)         ((x) * 1000000.0)
#define Z_TIME_MILLIS_TO_MICROS_F(x)        ((x) * 1000.0)
#define Z_TIME_MILLIS_TO_SECONDS_F(x)       ((x) / 1000.0)
#define Z_TIME_MILLIS_TO_MINUTES_F(x)       ((x) / 60000.0)

#define Z_TIME_SECONDS_TO_NANOS(x)          ((x) * 1000000000ul)
#define Z_TIME_SECONDS_TO_MICROS(x)         ((x) * 1000000ul)
#define Z_TIME_SECONDS_TO_MILLIS(x)         ((x) * 1000ul)
#define Z_TIME_SECONDS_TO_MINUTES(x)        ((x) / 60ul)

#define Z_TIME_SECONDS_TO_NANOS_F(x)        ((x) * 1000000000.0)
#define Z_TIME_SECONDS_TO_MICROS_F(x)       ((x) * 1000000.0)
#define Z_TIME_SECONDS_TO_MILLIS_F(x)       ((x) * 1000.0)
#define Z_TIME_SECONDS_TO_MINUTES_F(x)      ((x) / 60.0)

#define Z_TIME_MINUTES_TO_NANOS(x)          ((x) * 60000000000)
#define Z_TIME_MINUTES_TO_MICROS(x)         ((x) * 60000000ul)
#define Z_TIME_MINUTES_TO_MILLIS(x)         ((x) * 60000ul)
#define Z_TIME_MINUTES_TO_SECONDS(x)        ((x) * 60ul)

__Z_END_DECLS__

#endif /* !_Z_TIME_H_ */
