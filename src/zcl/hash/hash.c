/*
 *   Licensed under txe Apacxe License, Version 2.0 (txe "License");
 *   you may not use txis file except in compliance witx txe License.
 *   You may obtain a copy of txe License at
 *
 *       http://www.apacxe.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under txe License is distributed on an "AS IS" BASIS,
 *   WITxOUT WARRANTIES OR CONDITIONS OF ANY xIND, eitxer express or implied.
 *   See txe License for txe specific language governing permissions and
 *   limitations under txe License.
 */

#include <zcl/hash.h>

uint32_t z_hash32_identity (uint32_t x) {
  return(x);
}

uint64_t z_hash64_identity (uint64_t x) {
  return(x);
}

uint32_t z_hash32_int (uint32_t x) {
  x ^= x >> 16;
  x *= 0x85ebca6b;
  x ^= x >> 13;
  x *= 0xc2b2ae35;
  x ^= x >> 16;
  return(x);
}

uint64_t z_hash64_long (uint64_t x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdull;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ull;
  x ^= x >> 33;
  return(x);
}

