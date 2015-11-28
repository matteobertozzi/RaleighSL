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

uint64_t z_time_millis (void);
uint64_t z_time_micros (void);

uint64_t z_time_monotonic (void);

#define  Z_TIME_USEC(x)      ((x) * 1000ul)
#define  Z_TIME_MSEC(x)      ((x) * 1000000ul)
#define  Z_TIME_SEC(x)       ((x) * 1000000000ul)

#define  z_nano_to_sec(t)    ((t) * 0.000000001)
#define  z_nano_to_milli(t)  ((t) * 0.000001)
#define  z_nano_to_usec(t)   ((t) * 0.001)

void     z_time_usleep    (uint64_t usec);

__Z_END_DECLS__

#endif /* _Z_TIME_H_ */

