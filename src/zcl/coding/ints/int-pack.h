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

#ifndef _Z_INT_PACK_H_
#define _Z_INT_PACK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

int  z_uint8_pack1     (uint8_t *buffer, const uint8_t v);
int  z_uint8_pack2     (uint8_t *buffer, const uint8_t v[2]);
int  z_uint8_pack3     (uint8_t *buffer, const uint8_t v[3]);
int  z_uint8_pack4     (uint8_t *buffer, const uint8_t v[4]);

int  z_uint16_pack1    (uint8_t *buffer, const uint16_t v);
int  z_uint16_pack2    (uint8_t *buffer, const uint16_t v[2]);
int  z_uint16_pack3    (uint8_t *buffer, const uint16_t v[3]);
int  z_uint16_pack4    (uint8_t *buffer, const uint16_t v[4]);

int  z_uint32_pack1    (uint8_t *buffer, const uint32_t v);
int  z_uint32_pack2    (uint8_t *buffer, const uint32_t v[2]);
int  z_uint32_pack3    (uint8_t *buffer, const uint32_t v[3]);
int  z_uint32_pack4    (uint8_t *buffer, const uint32_t v[4]);

int  z_uint64_pack1    (uint8_t *buffer, const uint64_t v);
int  z_uint64_pack2    (uint8_t *buffer, const uint64_t v[2]);
int  z_uint64_pack3    (uint8_t *buffer, const uint64_t v[3]);
int  z_uint64_pack4    (uint8_t *buffer, const uint64_t v[4]);

void z_uint8_unpack4   (const uint8_t *buffer, uint8_t v[4]);
void z_uint16_unpack4  (const uint8_t *buffer, uint16_t v[4]);
void z_uint32_unpack4  (const uint8_t *buffer, uint32_t v[4]);
void z_uint64_unpack4  (const uint8_t *buffer, uint64_t v[4]);

int  z_uint8_unpack4c  (const uint8_t *buffer, uint32_t length, uint8_t v[4]);
int  z_uint16_unpack4c (const uint8_t *buffer, uint32_t length, uint16_t v[4]);
int  z_uint32_unpack4c (const uint8_t *buffer, uint32_t length, uint32_t v[4]);
int  z_uint64_unpack4c (const uint8_t *buffer, uint32_t length, uint64_t v[4]);

__Z_END_DECLS__

#endif /* !_Z_INT_PACK_H_ */
