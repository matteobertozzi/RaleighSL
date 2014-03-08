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

#ifndef _Z_DATA_FIELD_CODING_H_
#define _Z_DATA_FIELD_CODING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

int     z_field_encode (uint8_t *buf, uint16_t field_id, uint64_t length);
uint8_t z_field_encoded_length (uint16_t field_id, uint64_t length);
int     z_field_decode (const uint8_t *buf,
                        unsigned int buflen,
                        uint16_t *field_id,
                        uint64_t *length)
__Z_END_DECLS__

#endif /* _Z_DATA_FIELD_CODING_H_ */
