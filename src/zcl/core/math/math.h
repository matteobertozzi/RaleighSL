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

#ifndef _Z_MATH_H_
#define _Z_MATH_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

uint32_t z_align_pow2     (uint32_t n);
uint32_t z_nearest_pow2   (uint32_t n);

uint32_t z_ilog2          (uint32_t n);
uint8_t  z_bshift         (uint32_t min_block);

uint32_t z_sqrt32         (uint32_t x);

int      z_is_prime       (uint32_t candidate);
uint32_t z_prime32        (uint32_t min);
uint32_t z_lprime32       (uint32_t min);

uint32_t z_cycle32_prime  (uint32_t seq_size);

uint32_t z_cycle32        (uint32_t *seed, uint32_t prime, uint32_t seq_size);
uint64_t z_cycle64        (uint64_t *seed, uint64_t prime, uint64_t seq_size);

#define z_fast_u32_div10(x) ({                                              \
  uint64_t n = x;                                                           \
  uint64_t q = (n >> 1) + (n >> 2);                                         \
  q = q + (q >> 4);                                                         \
  q = q + (q >> 8);                                                         \
  q = (q + (q >> 16)) >> 3;                                                 \
  q + (((n - q * 10) + 6) >> 4);                                            \
})

#define z_fast_u32_div100(x) ({                                             \
  uint64_t n = x;                                                           \
  uint64_t q = (n >> 1) + (n >> 3) + (n >> 6) - (n >> 10) +                 \
               (n >> 12) + (n >> 13) - (n >> 16);                           \
  q = (q + (q >> 20)) >> 6;                                                 \
  q + (((n - q * 100) + 28) >> 7);                                          \
})

#define z_fast_u32_div1000(x) ({                                            \
  uint64_t n = x;                                                           \
  uint64_t t = (n >> 7) + (n >> 8) + (n >> 12);                             \
  uint64_t q = ((n >> 1) + t + (n >> 15) + (t >> 11) + (t >> 14)) >> 9;     \
  q + (((n - q * 1000) + 24) >> 10);                                        \
})

__Z_END_DECLS__

#endif /* !_Z_MATH_H_ */
