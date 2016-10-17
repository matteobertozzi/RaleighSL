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

#ifndef _Z_VINT_CODING_H_
#define _Z_VINT_CODING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

uint8_t  z_vint32_size   (uint32_t value);
uint8_t  z_vint64_size   (uint64_t value);

uint8_t *z_vint32_encode (uint8_t *buf, uint32_t value);
uint8_t *z_vint64_encode (uint8_t *buf, uint64_t value);

uint8_t  z_vint32_decode (const uint8_t *buf, uint32_t *value);
uint8_t  z_vint64_decode (const uint8_t *buf, uint64_t *value);

__Z_END_DECLS__

#endif /* !_Z_VINT_CODING_H_ */
