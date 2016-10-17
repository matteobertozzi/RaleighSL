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

#ifndef _Z_FIELD_CODING_H_
#define _Z_FIELD_CODING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

int z_field_write        (uint8_t *buf, uint16_t field_id, int vlength);
int z_field_write_uint8  (uint8_t *buf, uint16_t field_id, uint8_t value);
int z_field_write_uint16 (uint8_t *buf, uint16_t field_id, uint16_t value);
int z_field_write_uint32 (uint8_t *buf, uint16_t field_id, uint32_t value);
int z_field_write_uint64 (uint8_t *buf, uint16_t field_id, uint64_t value);

__Z_END_DECLS__

#endif /* !_Z_FIELD_CODING_H_ */
