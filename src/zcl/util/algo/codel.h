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

#ifndef _Z_CODEL_H_
#define _Z_CODEL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_codel {
  uint32_t next_ts;
  uint8_t  reset_delay;
  uint8_t  overloaded;
  uint16_t interval;
  uint16_t min_delay;
  uint16_t target_delay;
} z_codel_t;

void z_codel_init          (z_codel_t *self,
                            uint16_t interval,
                            uint16_t target_delay);
int  z_codel_is_overloaded (z_codel_t *self,
                            const uint64_t call_delay);

__Z_END_DECLS__

#endif /* !_Z_CODEL_H_ */
