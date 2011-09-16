/*
 *   Copyright 2011 Matteo Bertozzi
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

#ifndef _Z_BITOPS_H_
#define _Z_BITOPS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <stdint.h>

/* Find first bit set */
unsigned int    z_ffs8          (uint8_t x);
unsigned int    z_ffs16         (uint16_t x);
unsigned int    z_ffs32         (uint32_t x);
unsigned int    z_ffs64         (uint64_t x);

/* Find first zero bit set */
#define         z_ffz8(x)       z_ffs8(~(x))
#define         z_ffz16(x)      z_ffs16(~(x))
#define         z_ffz32(x)      z_ffs32(~(x))
#define         z_ffz64(x)      z_ffs64(~(x))

unsigned int    z_ffs           (const void *map,
                                 unsigned int offset,
                                 unsigned int size);
unsigned int    z_ffz           (const void *map,
                                 unsigned int offset,
                                 unsigned int size);

unsigned int    z_ffs_area      (const void *map,
                                 unsigned int offset,
                                 unsigned int size,
                                 unsigned int nr);
unsigned int    z_ffz_area      (const void *map,
                                 unsigned int offset,
                                 unsigned int size,
                                 unsigned int nr);

#define Z_BITMASK(n)            (1 << ((n) & 0x7))
#define Z_BIT(map, n)           (*(((uint8_t *)(map)) + ((n) >> 3)))

#define z_bit_test(map, n)      (!!(Z_BIT(map, n) & Z_BITMASK(n)))
#define z_bit_set(map, n)       do { Z_BIT(map, n) |=  Z_BITMASK(n); } while (0)
#define z_bit_clear(map, n)     do { Z_BIT(map, n) &= ~Z_BITMASK(n); } while (0)
#define z_bit_toggle(map, n)    do { Z_BIT(map, n) ^=  Z_BITMASK(n); } while (0)
#define z_bit_change(map, n, v)                                               \
    do {                                                                      \
        Z_BIT(map, n) = ((Z_BIT(map,n) & ~Z_BITMASK(n))|((!!(v))<<(n & 7)));  \
    } while (0)

__Z_END_DECLS__

#endif /* !_Z_BITOPS_H_ */

