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

#define z_timeval_to_micros(x)    ((x)->tv_sec * 1000000U + (x)->tv_usec)
#define z_usec_to_sec(t)          ((t) / 1000000u)

uint32_t z_time_secs    (void);
uint64_t z_time_millis  (void);
uint64_t z_time_micros  (void);
uint64_t z_time_nanos   (void);

void     z_time_usleep  (uint64_t usec);

__Z_END_DECLS__

#endif /* !_Z_TIME_H_ */
