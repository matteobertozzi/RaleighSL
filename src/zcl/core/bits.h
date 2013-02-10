/*
 *   Copyright 2011-2013 Matteo Bertozzi
 *
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

#define z_fetch_bits(byte, nbits, pos)                                         \
    (((byte) & (((1 << (nbits)) - 1) << (pos))) >> (pos))

#define z_fetch_7bit(byte, pos)       z_fetch_bits(byte, 7, pos)
#define z_fetch_6bit(byte, pos)       z_fetch_bits(byte, 6, pos)
#define z_fetch_5bit(byte, pos)       z_fetch_bits(byte, 5, pos)
#define z_fetch_4bit(byte, pos)       z_fetch_bits(byte, 4, pos)
#define z_fetch_3bit(byte, pos)       z_fetch_bits(byte, 3, pos)
#define z_fetch_2bit(byte, pos)       z_fetch_bits(byte, 2, pos)

#define z_rotl(x, r, s)               (((x) << (r)) | ((x) >> ((s) - (r))))
#define z_rotr(x, r, s)               (((x) >> (r)) | ((x) << ((s) - (r))))
#define z_rotl32(x, r)                z_rotl(x, r, 32)
#define z_rotl64(x, r)                z_rotl(x, r, 64)
#define z_rotr32(x, r)                z_rotr(x, r, 32)
#define z_rotr64(x, r)                z_rotr(x, r, 64)

#define Z_BITMASK(n)                  (1 << ((n) & 0x7))
#define Z_BIT(map, n)                 (*(((unsigned char *)(map)) + ((n) >> 3)))

#define z_bit_test(map, n)            (!!(Z_BIT(map, n) & Z_BITMASK(n)))

#define z_bit_set(map, n)                                                      \
    do { Z_BIT(map, n) |= Z_BITMASK(n); } while (0)

#define z_bit_clear(map, n)                                                    \
    do { Z_BIT(map, n) &= ~Z_BITMASK(n); } while (0)

#define z_bit_toggle(map, n)                                                   \
    do { Z_BIT(map, n) ^= Z_BITMASK(n); } while (0)

#define z_bit_change(map, n, v)                                                \
    do {                                                                       \
        Z_BIT(map, n) = (Z_BIT(map,n) & ~Z_BITMASK(n)) | ((!!(v)) << (n & 7)); \
    } while (0)

__Z_END_DECLS__

#endif /* !_Z_BITS_H_ */
