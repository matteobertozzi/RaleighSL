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

#ifndef _Z_DATA_INT_CODING_H_
#define _Z_DATA_INT_CODING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/mem.h>

uint8_t z_uint16_size (uint16_t value);
uint8_t z_uint32_size (uint32_t value);
uint8_t z_uint64_size (uint64_t value);

#define       z_uint8_size        z_uint16_size

#define       z_int8_size         z_int32_size
#define       z_int16_size        z_int32_size
#define       z_int32_size(n)     z_uint32_size(z_zigzag32_encode(n))
#define       z_int64_size(n)     z_uint64_size(z_zigzag64_encode(n))

#define z_uint16_encode     z_uint_encode
#define z_uint32_encode     z_uint_encode
#define z_uint64_encode     z_uint_encode

#if Z_CPU_IS_BIG_ENDIAN
  void z_encode_uint   (uint8_t *buf,
                        unsigned int length,
                        uint64_t value);

  void z_decode_uint16 (const uint8_t *buffer,
                        unsigned int length,
                        uint16_t *value);
  void z_decode_uint32 (const uint8_t *buffer,
                        unsigned int length,
                        uint32_t *value);
  void z_uint64_decode (const uint8_t *buffer,
                        unsigned int length,
                        uint64_t *value);
#else
  #define z_uint_encode(buf, len, val)       z_memcpy(buf, &(val), len)

  #define z_uint16_decode                    z_uint64_decode
  #define z_uint32_decode                    z_uint64_decode
  #define z_uint64_decode(buf, len, val)     *(val) = 0; z_memcpy(val, buf, len)
#endif


uint8_t z_vint32_size (uint32_t value);
uint8_t z_vint64_size (uint64_t value);

uint8_t *z_vint32_encode (uint8_t *buf, uint32_t value);
uint8_t *z_vint64_encode (uint8_t *buf, uint64_t value);
uint8_t  z_vint32_decode (const uint8_t *buf, uint32_t *value);
uint8_t  z_vint64_decode (const uint8_t *buf, uint64_t *value);

#define z_zigzag32_encode(n)      (((n) << 1) ^ ((n) >> 31))
#define z_zigzag64_encode(n)      (((n) << 1) ^ ((n) >> 63))
#define z_zigzag32_decode(n)      (((n) >> 1) ^ -((n) & 1))
#define z_zigzag64_decode(n)      (((n) >> 1) ^ -((n) & 1))

int z_3int_encode (uint8_t *buf, uint64_t a, uint64_t b, uint32_t c);
int z_3int_decode (const uint8_t *buf, uint64_t *a, uint64_t *b, uint32_t *c);

int z_4int_encode (uint8_t *buf,
                   uint32_t a,uint32_t b, uint32_t c, uint32_t d);
int z_4int_decode (const uint8_t *buf,
                   uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d);

__Z_END_DECLS__

#endif /* _Z_DATA_INT_CODING_H_ */
