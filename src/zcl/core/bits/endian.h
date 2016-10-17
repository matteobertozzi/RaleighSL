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

#ifndef _Z_ENDIAN_H_
#define _Z_ENDIAN_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define z_bswap16(x) ((uint16_t)(                                 \
  ((((uint16_t)(x)) & (uint16_t)0x00ffU) << 8) |                  \
  ((((uint16_t)(x)) & (uint16_t)0xff00U) >> 8)))

#define z_bswap32(x) ((uint32_t)(                                 \
  ((((uint32_t)(x)) & (uint32_t)0x000000ffUL) << 24) |            \
  ((((uint32_t)(x)) & (uint32_t)0x0000ff00UL) <<  8) |            \
  ((((uint32_t)(x)) & (uint32_t)0x00ff0000UL) >>  8) |            \
  ((((uint32_t)(x)) & (uint32_t)0xff000000UL) >> 24)))

#define z_bswap64(x) ((uint64_t)(                                 \
  ((((uint64_t)(x)) & (uint64_t)0x00000000000000ffULL) << 56) |   \
  ((((uint64_t)(x)) & (uint64_t)0x000000000000ff00ULL) << 40) |   \
  ((((uint64_t)(x)) & (uint64_t)0x0000000000ff0000ULL) << 24) |   \
  ((((uint64_t)(x)) & (uint64_t)0x00000000ff000000ULL) <<  8) |   \
  ((((uint64_t)(x)) & (uint64_t)0x000000ff00000000ULL) >>  8) |   \
  ((((uint64_t)(x)) & (uint64_t)0x0000ff0000000000ULL) >> 24) |   \
  ((((uint64_t)(x)) & (uint64_t)0x00ff000000000000ULL) >> 40) |   \
  ((((uint64_t)(x)) & (uint64_t)0xff00000000000000ULL) >> 56)))

#ifdef Z_CPU_IS_BIG_ENDIAN
  #define z_cpu_to_le16(x)        z_bswap16(x)
  #define z_cpu_to_be16(x)        (x)
  #define z_cpu_to_le32(x)        z_bswap32(x)
  #define z_cpu_to_be32(x)        (x)
  #define z_cpu_to_le64(x)        z_bswap64(x)
  #define z_cpu_to_be64(x)        (x)

  #define z_le16_to_cpu(x)        z_bswap16(x)
  #define z_be16_to_cpu(x)        (x)
  #define z_le32_to_cpu(x)        z_bswap32(x)
  #define z_be32_to_cpu(x)        (x)
  #define z_le64_to_cpu(x)        z_bswap64(x)
  #define z_be64_to_cpu(x)        (x)
#else
  #define z_cpu_to_le16(x)        (x)
  #define z_cpu_to_be16(x)        z_bswap16(x)
  #define z_cpu_to_le32(x)        (x)
  #define z_cpu_to_be32(x)        z_bswap32(x)
  #define z_cpu_to_le64(x)        (x)
  #define z_cpu_to_be64(x)        z_bswap64(x)

  #define z_le16_to_cpu(x)        (x)
  #define z_be16_to_cpu(x)        z_bswap16(x)
  #define z_le32_to_cpu(x)        (x)
  #define z_be32_to_cpu(x)        z_bswap32(x)
  #define z_le64_to_cpu(x)        (x)
  #define z_be64_to_cpu(x)        z_bswap64(x)
#endif

#ifdef Z_CPU_IS_BIG_ENDIAN
  #define z_get_uint32_le(n, b, i)                  \
    do {                                            \
      (n) = ((uint32_t) (b)[(i) + 0])               \
          | ((uint32_t) (b)[(i) + 1] <<  8)         \
          | ((uint32_t) (b)[(i) + 2] << 16)         \
          | ((uint32_t) (b)[(i) + 3] << 24);        \
    } while (0)

  #define z_put_uint32_le(n,b,i)                    \
    do {                                            \
      (b)[(i) + 0] = (uint8_t) ((n));               \
      (b)[(i) + 1] = (uint8_t) ((n) >>  8);         \
      (b)[(i) + 2] = (uint8_t) ((n) >> 16);         \
      (b)[(i) + 3] = (uint8_t) ((n) >> 24);         \
    } while (0)

  #define z_get_uint32_be(n, b, i)      (n) = *((uint32_t *)((b) + (i)))
  #define z_put_uint32_be(n, b, i)      *((uint32_t *)((b) + (i))) = (n)
  #define z_get_uint64_be(n, b, i)      (n) = *((uint64_t *)((b) + (i)))
  #define z_put_uint64_be(n, b, i)      *((uint64_t *)((b) + (i))) = (n)
#else
  #define z_get_uint32_le(n, b, i)      (n) = *((uint32_t *)((b) + (i)))
  #define z_put_uint32_le(n, b, i)      *((uint32_t *)((b) + (i))) = (n)
  #define z_get_uint64_le(n, b, i)      (n) = *((uint64_t *)((b) + (i)))
  #define z_put_uint64_le(n, b, i)      *((uint64_t *)((b) + (i))) = (n)

  #define z_get_uint32_be(n,b,i)                          \
    do {                                                  \
      (n) = ((uint32_t) (b)[(i) + 0] << 24)               \
          | ((uint32_t) (b)[(i) + 1] << 16)               \
          | ((uint32_t) (b)[(i) + 2] <<  8)               \
          | ((uint32_t) (b)[(i) + 3]);                    \
    } while (0)

  #define z_put_uint32_be(n,b,i)                          \
    do {                                                  \
      (b)[(i) + 0] = (uint8_t) ((n) >> 24);               \
      (b)[(i) + 1] = (uint8_t) ((n) >> 16);               \
      (b)[(i) + 2] = (uint8_t) ((n) >>  8);               \
      (b)[(i) + 3] = (uint8_t) ((n));                     \
    } while (0)

  #define z_get_uint64_be(n,b,i)                          \
    do {                                                  \
      (n) = ((uint64_t) (b)[(i) + 0] << 56)               \
          | ((uint64_t) (b)[(i) + 1] << 48)               \
          | ((uint64_t) (b)[(i) + 2] << 40)               \
          | ((uint64_t) (b)[(i) + 3] << 32)               \
          | ((uint64_t) (b)[(i) + 4] << 24)               \
          | ((uint64_t) (b)[(i) + 5] << 16)               \
          | ((uint64_t) (b)[(i) + 6] <<  8)               \
          | ((uint64_t) (b)[(i) + 7]);                    \
    } while (0)

  #define z_put_uint64_be(n,b,i)                          \
    do {                                                  \
      (b)[(i) + 0] = (uint8_t) ((n) >> 56);               \
      (b)[(i) + 1] = (uint8_t) ((n) >> 48);               \
      (b)[(i) + 2] = (uint8_t) ((n) >> 40);               \
      (b)[(i) + 3] = (uint8_t) ((n) >> 32);               \
      (b)[(i) + 4] = (uint8_t) ((n) >> 24);               \
      (b)[(i) + 5] = (uint8_t) ((n) >> 16);               \
      (b)[(i) + 6] = (uint8_t) ((n) >>  8);               \
      (b)[(i) + 7] = (uint8_t) ((n));                     \
    } while (0)
#endif

__Z_END_DECLS__

#endif /* !_Z_ENDIAN_H_ */
