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

#include <zcl/bits.h>

int z_bit_set_count32 (uint32_t v) {
  v = v - ((v >> 1) & 0x55555555);
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
  v = (v + (v >> 4)) & 0x0F0F0F0F;
  return (v * 0x01010101) >> 24;
}

int z_bit_set_count64 (uint64_t v) {
  v = v - ((v >> 1) & 0x5555555555555555ull);
  v = (v & 0x3333333333333333ull) + ((v >> 2) & 0x3333333333333333ull);
  v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0Full;
  return (v * 0x0101010101010101ull) >> 56;
}
