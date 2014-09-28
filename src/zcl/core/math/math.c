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

#include <zcl/math.h>
#include <math.h>

uint32_t z_align_pow2 (uint32_t n) {
  --n;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  return(n + 1);
}

uint32_t z_ilog2 (uint32_t n) {
  uint32_t lg2 = 0U;
  if (n & (n - 1)) lg2 += 1;
  if (n >> 16) { lg2 += 16; n >>= 16; }
  if (n >>  8) { lg2 +=  8; n >>= 8; }
  if (n >>  4) { lg2 +=  4; n >>= 4; }
  if (n >>  2) { lg2 +=  2; n >>= 2; }
  if (n >>  1) { lg2 +=  1; }
  return(lg2);
}

uint8_t z_bshift (uint32_t min_block) {
  uint8_t bshift = 0;
  while (min_block >>= 1)
    bshift++;
  return(bshift);
}

uint32_t z_cycle32 (uint32_t *seed, uint32_t prime, uint32_t seq_size) {
  *seed = (*seed + prime) % seq_size;
  return *seed;
}

uint64_t z_cycle64 (uint64_t *seed, uint64_t prime, uint64_t seq_size) {
  *seed = (*seed + prime) % seq_size;
  return *seed;
}

uint32_t z_cycle32_prime (uint32_t seq_size) {
  uint32_t i;
  for (i = 101; i < 0xffffffff; i += 2) {
    uint32_t mod = (i % seq_size);
    if ((mod != 0 && mod != i) && (z_is_prime(i) && ((i - 1) % 101) != 0)) {
      return i;
    }
  }
  for (i = 2; i < 101; i += 2) {
    uint32_t mod = (i % seq_size);
    if ((mod != 0 && mod != i) && (z_is_prime(i) && ((i - 1) % 101) != 0)) {
      return i;
    }
  }
  return(2);
}

uint32_t z_sqrt32 (uint32_t x) {
  return (uint32_t)sqrt(x);
}

int z_is_prime (uint32_t candidate) {
  if ((candidate & 1) != 0) {
    uint32_t limit = z_sqrt32(candidate);
    int d;
    for (d = 3; d <= limit; d += 2) {
      if ((candidate % d) == 0)
        return(0);
    }
    return(1);
  }
  return(candidate == 2);
}

uint32_t z_prime32 (uint32_t min) {
  uint32_t i;
  for (i = min | 1; i < 0xffffffff; i += 2) {
    if (z_is_prime(i) && ((i - 1) % 101) != 0) {
      return i;
    }
  }
  return min;
}

uint32_t z_lprime32 (uint32_t min) {
  uint32_t i;

  i = z_max(2, min) | 1;
  if (i <= min && (z_is_prime(i) && ((i - 1) % 101) != 0))
    return(i);

  for (i -= 2; i > 2; i -= 2) {
    if (z_is_prime(i) && ((i - 1) % 101) != 0) {
      return i;
    }
  }
  return 2;
}
