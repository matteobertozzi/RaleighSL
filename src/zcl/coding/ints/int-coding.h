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

#ifndef _Z_INT_CODING_H_
#define _Z_INT_CODING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define z_uint16_size(value)              (1 + ((value) > (1 << 8)))
uint8_t z_uint32_size (uint32_t value);
uint8_t z_uint64_size (uint64_t value);

#define z_int16_size(n)     z_int32_size(n)
#define z_int32_size(n)     z_uint32_size(z_zigzag32_encode(n))
#define z_int64_size(n)     z_uint64_size(z_zigzag64_encode(n))

#if defined(Z_CPU_IS_BIG_ENDIAN)
  void z_uint_encode   (uint8_t *buf,
                        unsigned int length,
                        uint64_t value);

  #define z_uint16_encode z_uint_encode
  #define z_uint32_encode z_uint_encode
  #define z_uint64_encode z_uint_encode

  void z_uint16_decode (const uint8_t *buffer,
                        unsigned int length,
                        uint16_t *value);
  void z_uint32_decode (const uint8_t *buffer,
                        unsigned int length,
                        uint32_t *value);
  void z_uint64_decode (const uint8_t *buffer,
                        unsigned int length,
                        uint64_t *value);
#else
  #define z_uint_encode(buf, len, val)     z_memcpy64(buf, &(val), len)
  #define z_uint16_encode(buf, len, val)   z_memcpy16(buf, &(val), len)
  #define z_uint32_encode(buf, len, val)   z_memcpy32(buf, &(val), len)
  #define z_uint64_encode(buf, len, val)   z_memcpy64(buf, &(val), len)

  #define z_uint16_decode(buf, len, val)   *(val) = 0; z_memcpy16(val, buf, len)
  #define z_uint32_decode(buf, len, val)   *(val) = 0; z_memcpy32(val, buf, len)
  #define z_uint64_decode(buf, len, val)   *(val) = 0; z_memcpy64(val, buf, len)
#endif

#define z_zigzag32_encode(n)      (((n) << 1) ^ ((n) >> 31))
#define z_zigzag64_encode(n)      (((n) << 1) ^ ((n) >> 63))
#define z_zigzag32_decode(n)      (((n) >> 1) ^ -((n) & 1))
#define z_zigzag64_decode(n)      (((n) >> 1) ^ -((n) & 1))

uint8_t   z_vint32_size   (uint32_t value);
uint8_t   z_vint64_size   (uint64_t value);

uint8_t * z_vint32_encode (uint8_t *buf, uint32_t value);
uint8_t * z_vint64_encode (uint8_t *buf, uint64_t value);
uint8_t   z_vint32_decode (const uint8_t *buf, uint32_t *value);
uint8_t   z_vint64_decode (const uint8_t *buf, uint64_t *value);

__Z_END_DECLS__

#endif /* !_Z_INT_CODING_H_ */
