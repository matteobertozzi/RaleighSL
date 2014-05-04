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
 *  Bytes Required to encode an integer
 */
uint8_t z_uint16_size (uint16_t value) {
  return(1 + (value < (1ul << 8)));
}

uint8_t z_uint32_size (uint32_t value) {
  if (value < (1ul <<  8)) return(1);
  if (value < (1ul << 16)) return(2);
  if (value < (1ul << 24)) return(3);
  return(4);
}

uint8_t z_uint64_size (uint64_t value) {
  if (value < (1ull << 32)) {
    if (value < (1ull <<  8)) return(1);
    if (value < (1ull << 16)) return(2);
    if (value < (1ull << 24)) return(3);
    return(4);
  } else {
    if (value < (1ull << 40)) return(5);
    if (value < (1ull << 48)) return(6);
    if (value < (1ull << 56)) return(7);
    return(8);
  }
}

/* ============================================================================
 *  Encode/Decode unsigned integer
 */
#if defined(Z_CPU_IS_BIG_ENDIAN)
void z_uint_encode (uint8_t *buf, unsigned int length, uint64_t value) {
  switch (length) {
    case 8: buf[7] = (value >> 56) & 0xff;
    case 7: buf[6] = (value >> 48) & 0xff;
    case 6: buf[5] = (value >> 40) & 0xff;
    case 5: buf[4] = (value >> 32) & 0xff;
    case 4: buf[3] = (value >> 24) & 0xff;
    case 3: buf[2] = (value >> 16) & 0xff;
    case 2: buf[1] = (value >>  8) & 0xff;
    case 1: buf[0] = value & 0xff;
  }
}

void z_uint16_decode (const uint8_t *buffer,
                      unsigned int length, uint16_t *value)
{
  uint32_t result = 0;

  switch (length) {
    case 2: result += buffer[1] <<  8;
    case 1: result += buffer[0];
  }

  *value = result;
}

void z_uint32_decode (const uint8_t *buffer,
                      unsigned int length,
                      uint32_t *value)
{
  uint32_t result = 0;

  switch (length) {
    case 4: result  = ((uint64_t)buffer[3]) << 24;
    case 3: result += buffer[2] << 16;
    case 2: result += buffer[1] <<  8;
    case 1: result += buffer[0];
  }

  *value = result;
}

void z_uint64_decode (const uint8_t *buffer,
                      unsigned int length,
                      uint64_t *value)
{
  uint64_t result = 0;

  switch (length) {
    case 8: result  = z_shl56(buffer[7]);
    case 7: result += z_shl48(buffer[6]);
    case 6: result += z_shl40(buffer[5]);
    case 5: result += z_shl32(buffer[4]);
    case 4: result += z_shl24(buffer[3]);
    case 3: result += buffer[2] << 16;
    case 2: result += buffer[1] <<  8;
    case 1: result += buffer[0];
  }

  *value = result;
}
#endif /* Z_CPU_IS_BIG_ENDIAN */
