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

#define z_mask_bits(nbits, pos)         (((1 << (nbits)) - 1) << ((pos) & 0x7))
#define z_mask_7bit(pos)                z_mask_bits(7, pos)
#define z_mask_6bit(pos)                z_mask_bits(6, pos)
#define z_mask_5bit(pos)                z_mask_bits(5, pos)
#define z_mask_4bit(pos)                z_mask_bits(4, pos)
#define z_mask_3bit(pos)                z_mask_bits(3, pos)
#define z_mask_2bit(pos)                z_mask_bits(2, pos)
#define z_mask_1bit(pos)                (1 << ((pos) & 0x7))

#define z_fetch_bits(byte, nbits, pos)                                       \
  (((byte) & z_mask_bits(nbits, pos)) >> (pos))

#define z_fetch_7bit(byte, pos)         z_fetch_bits(byte, 7, pos)
#define z_fetch_6bit(byte, pos)         z_fetch_bits(byte, 6, pos)
#define z_fetch_5bit(byte, pos)         z_fetch_bits(byte, 5, pos)
#define z_fetch_4bit(byte, pos)         z_fetch_bits(byte, 4, pos)
#define z_fetch_3bit(byte, pos)         z_fetch_bits(byte, 3, pos)
#define z_fetch_2bit(byte, pos)         z_fetch_bits(byte, 2, pos)
#define z_fetch_1bit(byte, pos)         ((byte) & z_mask_1bit(pos))

#define z_change_bits(byte, nbits, pos, value)                                 \
  (byte) = (((byte) & ~z_fetch_bits(byte, nbits, pos)) | ((value) << ((pos) & 0x7)))

#define z_change_7bit(byte, pos, v)     z_change_bits(byte, 7, pos, v)
#define z_change_6bit(byte, pos, v)     z_change_bits(byte, 6, pos, v)
#define z_change_5bit(byte, pos, v)     z_change_bits(byte, 5, pos, v)
#define z_change_4bit(byte, pos, v)     z_change_bits(byte, 4, pos, v)
#define z_change_3bit(byte, pos, v)     z_change_bits(byte, 3, pos, v)
#define z_change_2bit(byte, pos, v)     z_change_bits(byte, 2, pos, v)
#define z_change_1bit(byte, pos, v)     z_change_bits(byte, 1, pos, v)

#define z_clear_bits(byte, nbits, pos)                                        \
  (byte) &= ~z_mask_bits(nbits, pos)

#define z_clear_7bit(byte, pos)         z_clear_bits(byte, 7, pos)
#define z_clear_6bit(byte, pos)         z_clear_bits(byte, 6, pos)
#define z_clear_5bit(byte, pos)         z_clear_bits(byte, 5, pos)
#define z_clear_4bit(byte, pos)         z_clear_bits(byte, 4, pos)
#define z_clear_3bit(byte, pos)         z_clear_bits(byte, 3, pos)
#define z_clear_2bit(byte, pos)         z_clear_bits(byte, 2, pos)
#define z_clear_1bit(byte, pos)         z_clear_bits(byte, 1, pos)

#define z_set_bits(byte, nbits, pos)                                          \
  (byte) |= z_mask_bits(nbits, pos)

#define z_set_7bit(byte, pos)           z_set_bits(byte, 7, pos)
#define z_set_6bit(byte, pos)           z_set_bits(byte, 6, pos)
#define z_set_5bit(byte, pos)           z_set_bits(byte, 5, pos)
#define z_set_4bit(byte, pos)           z_set_bits(byte, 4, pos)
#define z_set_3bit(byte, pos)           z_set_bits(byte, 3, pos)
#define z_set_2bit(byte, pos)           z_set_bits(byte, 2, pos)
#define z_set_1bit(byte, pos)           z_set_bits(byte, 1, pos)

__Z_END_DECLS__

#endif /* _Z_BITS_H_ */
