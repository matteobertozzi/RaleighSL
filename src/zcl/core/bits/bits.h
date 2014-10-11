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

#ifndef _Z_BITS_H_
#define _Z_BITS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define z_rotl(x, r, s)                 (((x) << (r)) | ((x) >> ((s) - (r))))
#define z_rotr(x, r, s)                 (((x) >> (r)) | ((x) << ((s) - (r))))
#define z_rotl32(x, r)                  z_rotl(x, r, 32)
#define z_rotl64(x, r)                  z_rotl(x, r, 64)
#define z_rotr32(x, r)                  z_rotr(x, r, 32)
#define z_rotr64(x, r)                  z_rotr(x, r, 64)

#define z_shl56(x)                      (((uint64_t)(x)) << 56)
#define z_shl48(x)                      (((uint64_t)(x)) << 48)
#define z_shl40(x)                      (((uint64_t)(x)) << 40)
#define z_shl32(x)                      (((uint64_t)(x)) << 32)
#define z_shl24(x)                      (((uint64_t)(x)) << 24)
#define z_shl16(x)                      ((x) << 16)
#define z_shl8(x)                       ((x) <<  8)

#define z_bits_mask(nbits, pos)         (((1 << (nbits)) - 1) << (pos))
#define z_7bit_mask(pos)                z_bits_mask(7, pos)
#define z_6bit_mask(pos)                z_bits_mask(6, pos)
#define z_5bit_mask(pos)                z_bits_mask(5, pos)
#define z_4bit_mask(pos)                z_bits_mask(4, pos)
#define z_3bit_mask(pos)                z_bits_mask(3, pos)
#define z_2bit_mask(pos)                z_bits_mask(2, pos)
#define z_1bit_mask(pos)                (1 << ((pos) & 0x7))

#define z_bits_fetch(byte, nbits, pos)                                       \
  (((byte) & z_bits_mask(nbits, pos)) >> (pos))

#define z_7bit_fetch(byte, pos)         z_bits_fetch(byte, 7, pos)
#define z_6bit_fetch(byte, pos)         z_bits_fetch(byte, 6, pos)
#define z_5bit_fetch(byte, pos)         z_bits_fetch(byte, 5, pos)
#define z_4bit_fetch(byte, pos)         z_bits_fetch(byte, 4, pos)
#define z_3bit_fetch(byte, pos)         z_bits_fetch(byte, 3, pos)
#define z_2bit_fetch(byte, pos)         z_bits_fetch(byte, 2, pos)
#define z_1bit_fetch(byte, pos)         ((byte) & z_1bit_mask(pos))

#define z_bits_change(byte, nbits, pos, value)                                 \
  (byte) = (((byte) & ~z_bits_mask(nbits, pos)) | ((value) << (pos)))

#define z_7bit_change(byte, pos, v)     z_bits_change(byte, 7, pos, v)
#define z_6bit_change(byte, pos, v)     z_bits_change(byte, 6, pos, v)
#define z_5bit_change(byte, pos, v)     z_bits_change(byte, 5, pos, v)
#define z_4bit_change(byte, pos, v)     z_bits_change(byte, 4, pos, v)
#define z_3bit_change(byte, pos, v)     z_bits_change(byte, 3, pos, v)
#define z_2bit_change(byte, pos, v)     z_bits_change(byte, 2, pos, v)
#define z_1bit_change(byte, pos, v)     z_bits_change(byte, 1, pos, v)

#define z_bits_clear(byte, nbits, pos)                                        \
  (byte) &= ~z_bits_mask(nbits, pos)

#define z_7bit_clear(byte, pos)         z_bits_clear(byte, 7, pos)
#define z_6bit_clear(byte, pos)         z_bits_clear(byte, 6, pos)
#define z_5bit_clear(byte, pos)         z_bits_clear(byte, 5, pos)
#define z_4bit_clear(byte, pos)         z_bits_clear(byte, 4, pos)
#define z_3bit_clear(byte, pos)         z_bits_clear(byte, 3, pos)
#define z_2bit_clear(byte, pos)         z_bits_clear(byte, 2, pos)
#define z_1bit_clear(byte, pos)         z_bits_clear(byte, 1, pos)

#define z_bits_set(byte, nbits, pos)                                          \
  (byte) |= z_bits_mask(nbits, pos)

#define z_7bit_set(byte, pos)           z_bits_set(byte, 7, pos)
#define z_6bit_set(byte, pos)           z_bits_set(byte, 6, pos)
#define z_5bit_set(byte, pos)           z_bits_set(byte, 5, pos)
#define z_4bit_set(byte, pos)           z_bits_set(byte, 4, pos)
#define z_3bit_set(byte, pos)           z_bits_set(byte, 3, pos)
#define z_2bit_set(byte, pos)           z_bits_set(byte, 2, pos)
#define z_1bit_set(byte, pos)           z_bits_set(byte, 1, pos)

int z_bit_set_count32 (uint32_t v);
int z_bit_set_count64 (uint64_t v);

__Z_END_DECLS__

#endif /* _Z_BITS_H_ */
