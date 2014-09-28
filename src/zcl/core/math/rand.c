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

#include <zcl/macros.h>

uint32_t z_rand32 (uint64_t *seed) {
  uint64_t next;
  next = (*seed * 0x5DEECE66DL + 0xBL) & 0xffffffffffff;
  *seed = next;
  return (next >> 16) & 0xffffffff;
}

uint64_t z_rand64 (uint64_t *seed) {
  uint64_t rslt;
  uint64_t next;

  next  = (*seed * 0x5DEECE66DL + 0xBL) & 0xffffffffffff;
  rslt  = (next >> 16) << 32;
  next  = (next  * 0x5DEECE66DL + 0xBL) & 0xffffffffffff;
  rslt |= (next >> 16);
  *seed = next;

  return rslt;
}

void z_rand_bytes (uint64_t *seed, uint8_t *bytes, size_t length) {
  uint8_t *p = bytes;

  while (length >= 4) {
    uint32_t rnd = z_rand32(seed);
    *p++ = (rnd >>  0) & 0xff;
    *p++ = (rnd >>  8) & 0xff;
    *p++ = (rnd >> 16) & 0xff;
    *p++ = (rnd >> 24) & 0xff;
    length -= 4;
  }

  if (length > 0) {
    uint32_t rnd = z_rand32(seed);
    switch (length) {
      case 3: *p++ = (rnd >>  0) & 0xff;
      case 2: *p++ = (rnd >>  8) & 0xff;
      case 1: *p   = (rnd >> 16) & 0xff;
    }
  }
}

void z_rand_uuid (uint64_t *seed, uint8_t uuid[16]) {
  z_rand_bytes(seed, uuid, 16);
  uuid[6] &= 0x0f;  /* clear version       */
  uuid[6] |= 0x40;  /* set to version 4    */
  uuid[8] &= 0x3f;  /* clear variant       */
  uuid[8] |= 0x80;  /* set to IETF variant */
}
