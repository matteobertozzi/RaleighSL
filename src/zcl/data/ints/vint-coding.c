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

#include <zcl/int-coding.h>

/* ============================================================================
 *  Bytes Required to encode a vint
 */
uint8_t z_vint32_size (uint32_t value) {
  if (value < (1ull <<  7)) return(1);
  if (value < (1ull << 14)) return(2);
  if (value < (1ull << 21)) return(3);
  if (value < (1ull << 28)) return(4);
  return(5);
}

uint8_t z_vint64_size (uint64_t value) {
  if (value < (1ull << 35)) {
    if (value < (1ull <<  7)) return(1);
    if (value < (1ull << 14)) return(2);
    if (value < (1ull << 21)) return(3);
    if (value < (1ull << 28)) return(4);
    return(5);
  } else {
    if (value < (1ull << 42)) return(6);
    if (value < (1ull << 49)) return(7);
    if (value < (1ull << 56)) return(8);
    if (value < (1ull << 63)) return(9);
    return(10);
  }
}

/* ============================================================================
 *  Encode/Decode variable-length
 */
uint8_t *z_vint32_encode (uint8_t *buf, uint32_t value) {
  if (value < (1 << 7)) {
    *buf++ = value;
  } else if (value < (1 << 14)) {
    *buf++ = value | 128;
    *buf++ = (value >> 7);
  } else if (value < (1 << 21)) {
    *buf++ =  value | 128;
    *buf++ = (value >>  7) | 128;
    *buf++ = (value >> 14);
  } else if (value < (1 << 28)) {
    *buf++ =  value | 128;
    *buf++ = (value >> 7) | 128;
    *buf++ = (value >> 14) | 128;
    *buf++ = (value >> 21);
  } else {
    *buf++ =  value | 128;
    *buf++ = (value >>  7) | 128;
    *buf++ = (value >> 14) | 128;
    *buf++ = (value >> 21) | 128;
    *buf++ = (value >> 28);
  }
  return(buf);
}

uint8_t *z_vint64_encode (uint8_t *buf, uint64_t value) {
  while (value >= 128) {
    *buf++ = (value & 0x7f) | 128;
    value >>= 7;
  }
  *buf++ = value & 0x7f;
  return(buf);
}

uint8_t z_vint32_decode (const uint8_t *buf, uint32_t *value) {
  unsigned int shift = 0;
  uint32_t result = 0;
  int count = 1;

  if ((*buf & 128) == 0) {
    *value = *buf;
    return(1);
  }

  while (shift <= 28) {
    uint64_t b = *buf++;
    if (b & 128) {
      result |= (b & 0x7f) << shift;
    } else {
      result |= (b << shift);
      *value = result;
      return(count);
    }
    shift += 7;
    ++count;
  }

  /* Never reached */
  return(0);
}

uint8_t z_vint64_decode (const uint8_t *buf, uint64_t *value) {
  unsigned int shift = 0;
  uint64_t result = 0;
  int count = 1;

  if ((*buf & 128) == 0) {
    *value = *buf;
    return(1);
  }

  while (shift <= 63) {
    uint64_t b = *buf++;
    if (b & 128) {
      result |= (b & 0x7f) << shift;
    } else {
      result |= (b << shift);
      *value = result;
      return(count);
    }
    shift += 7;
    ++count;
  }

  /* Never reached */
  return(0);
}