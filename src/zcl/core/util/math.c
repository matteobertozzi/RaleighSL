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