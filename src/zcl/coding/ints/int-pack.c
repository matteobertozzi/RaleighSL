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
#include <zcl/int-pack.h>

/* ============================================================================
 *  list uint8-packing
 */
int z_uint8_pack1 (uint8_t *buffer, uint8_t v) {
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = v;
  return(3);
}

int z_uint8_pack2 (uint8_t *buffer, const uint8_t v[2]) {
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = v[0];
  buffer[3] = 0;
  buffer[4] = v[1];
  return(5);
}

int z_uint8_pack3 (uint8_t *buffer, const uint8_t v[3]) {
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = v[0];
  buffer[3] = 0;
  buffer[4] = v[1];
  buffer[5] = 0;
  buffer[6] = v[2];
  return(7);
}

int z_uint8_pack4 (uint8_t *buffer, const uint8_t v[4]) {
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = v[0];
  buffer[3] = 0;
  buffer[4] = v[1];
  buffer[5] = 0;
  buffer[6] = v[2];
  buffer[7] = 0;
  buffer[8] = v[3];
  return(9);
}

/* ============================================================================
 *  list uint16-packing
 */
int z_uint16_pack1 (uint8_t *buffer, uint16_t v) {
  buffer[0] = 0;
  buffer[1] = (v >> 8);
  buffer[2] = (v & 0xff);
  return(3);
}

int z_uint16_pack2 (uint8_t *buffer, const uint16_t v[2]) {
  buffer[0] = 0;
  buffer[1] = (v[0] >> 8);
  buffer[2] = (v[0] & 0xff);
  buffer[3] = (v[1] >> 8);
  buffer[4] = (v[1] & 0xff);
  return(5);
}

int z_uint16_pack3 (uint8_t *buffer, const uint16_t v[3]) {
  buffer[0] = 0;
  buffer[1] = (v[0] >> 8);
  buffer[2] = (v[0] & 0xff);
  buffer[3] = (v[1] >> 8);
  buffer[4] = (v[1] & 0xff);
  buffer[5] = (v[2] >> 8);
  buffer[6] = (v[2] & 0xff);
  return(7);
}

int z_uint16_pack4 (uint8_t *buffer, const uint16_t v[4]) {
  buffer[0] = 0;
  buffer[1] = (v[0] >> 8);
  buffer[2] = (v[0] & 0xff);
  buffer[3] = (v[1] >> 8);
  buffer[4] = (v[1] & 0xff);
  buffer[5] = (v[2] >> 8);
  buffer[6] = (v[2] & 0xff);
  buffer[7] = (v[3] >> 8);
  buffer[8] = (v[3] & 0xff);
  return(9);
}

/* ============================================================================
 *  list uint32-packing
 */
int z_uint32_pack1 (uint8_t *buffer, uint32_t v) {
  uint8_t vlength;

  vlength = z_uint32_size(v);
  vlength = z_align_up(vlength, 2);

  *buffer++ = (vlength >> 1) - 1;
  z_uint32_encode(buffer, vlength, v);
  return(vlength + 1);
}

int z_uint32_pack2 (uint8_t *buffer, const uint32_t v[2]) {
  uint8_t *pbuffer = buffer;
  uint8_t vlength[2];

  vlength[0] = z_uint32_size(v[0]); vlength[0] = z_align_up(vlength[0], 2);
  vlength[1] = z_uint32_size(v[1]); vlength[1] = z_align_up(vlength[1], 2);

  *pbuffer++ = ((vlength[1] >> 1) - 1) << 2 | ((vlength[0] >> 1) - 1);
  z_uint32_encode(pbuffer, vlength[0], v[0]); pbuffer += vlength[0];
  z_uint32_encode(pbuffer, vlength[1], v[1]); pbuffer += vlength[1];
  return(pbuffer - buffer);
}

int z_uint32_pack3 (uint8_t *buffer, const uint32_t v[3]) {
  uint8_t *pbuffer = buffer;
  uint8_t vlength[3];

  vlength[0] = z_uint32_size(v[0]); vlength[0] = z_align_up(vlength[0], 2);
  vlength[1] = z_uint32_size(v[1]); vlength[1] = z_align_up(vlength[1], 2);
  vlength[2] = z_uint32_size(v[2]); vlength[2] = z_align_up(vlength[2], 2);

  *pbuffer++ = ((vlength[2] >> 1) - 1) << 4 |
               ((vlength[1] >> 1) - 1) << 2 | ((vlength[0] >> 1) - 1);
  z_uint32_encode(pbuffer, vlength[0], v[0]); pbuffer += vlength[0];
  z_uint32_encode(pbuffer, vlength[1], v[1]); pbuffer += vlength[1];
  z_uint32_encode(pbuffer, vlength[2], v[2]); pbuffer += vlength[2];
  return(pbuffer - buffer);
}

int z_uint32_pack4 (uint8_t *buffer, const uint32_t v[4]) {
  uint8_t *pbuffer = buffer;
  uint8_t vlength[4];

  vlength[0] = z_uint32_size(v[0]); vlength[0] = z_align_up(vlength[0], 2);
  vlength[1] = z_uint32_size(v[1]); vlength[1] = z_align_up(vlength[1], 2);
  vlength[2] = z_uint32_size(v[2]); vlength[2] = z_align_up(vlength[2], 2);
  vlength[3] = z_uint32_size(v[3]); vlength[3] = z_align_up(vlength[3], 2);

  *pbuffer++ = ((vlength[3] >> 1) - 1) << 6 | ((vlength[2] >> 1) - 1) << 4 |
               ((vlength[1] >> 1) - 1) << 2 | ((vlength[0] >> 1) - 1);
  z_uint32_encode(pbuffer, vlength[0], v[0]); pbuffer += vlength[0];
  z_uint32_encode(pbuffer, vlength[1], v[1]); pbuffer += vlength[1];
  z_uint32_encode(pbuffer, vlength[2], v[2]); pbuffer += vlength[2];
  z_uint32_encode(pbuffer, vlength[3], v[3]); pbuffer += vlength[3];
  return(pbuffer - buffer);
}

/* ============================================================================
 *  list uint64-packing
 */
int z_uint64_pack1 (uint8_t *buffer, uint64_t v) {
  uint8_t vlength;

  vlength = z_uint64_size(v);
  vlength = z_align_up(vlength, 2);

  *buffer++ = (vlength >> 1) - 1;
  z_uint64_encode(buffer, vlength, v);
  return(vlength + 1);
}

int z_uint64_pack2 (uint8_t *buffer, const uint64_t v[2]) {
  uint8_t *pbuffer = buffer;
  uint8_t vlength[2];

  vlength[0] = z_uint64_size(v[0]); vlength[0] = z_align_up(vlength[0], 2);
  vlength[1] = z_uint64_size(v[1]); vlength[1] = z_align_up(vlength[1], 2);

  *pbuffer++ = ((vlength[1] >> 1) - 1) << 2 | ((vlength[0] >> 1) - 1);

  z_uint64_encode(pbuffer, vlength[0], v[0]); pbuffer += vlength[0];
  z_uint64_encode(pbuffer, vlength[1], v[1]); pbuffer += vlength[1];
  return(pbuffer - buffer);
}

int z_uint64_pack3 (uint8_t *buffer, const uint64_t v[3]) {
  uint8_t *pbuffer = buffer;
  uint8_t vlength[3];

  vlength[0] = z_uint64_size(v[0]); vlength[0] = z_align_up(vlength[0], 2);
  vlength[1] = z_uint64_size(v[1]); vlength[1] = z_align_up(vlength[1], 2);
  vlength[2] = z_uint64_size(v[2]); vlength[2] = z_align_up(vlength[2], 2);

  *pbuffer++ = ((vlength[2] >> 1) - 1) << 4 |
               ((vlength[1] >> 1) - 1) << 2 | ((vlength[0] >> 1) - 1);
  z_uint64_encode(pbuffer, vlength[0], v[0]); pbuffer += vlength[0];
  z_uint64_encode(pbuffer, vlength[1], v[1]); pbuffer += vlength[1];
  z_uint64_encode(pbuffer, vlength[2], v[2]); pbuffer += vlength[2];
  return(pbuffer - buffer);
}

int z_uint64_pack4 (uint8_t *buffer, const uint64_t v[4]) {
  uint8_t *pbuffer = buffer;
  uint8_t vlength[4];

  vlength[0] = z_uint64_size(v[0]); vlength[0] = z_align_up(vlength[0], 2);
  vlength[1] = z_uint64_size(v[1]); vlength[1] = z_align_up(vlength[1], 2);
  vlength[2] = z_uint64_size(v[2]); vlength[2] = z_align_up(vlength[2], 2);
  vlength[3] = z_uint64_size(v[3]); vlength[3] = z_align_up(vlength[3], 2);

  *pbuffer++ = ((vlength[3] >> 1) - 1) << 6 | ((vlength[2] >> 1) - 1) << 4 |
               ((vlength[1] >> 1) - 1) << 2 | ((vlength[0] >> 1) - 1);

  z_uint64_encode(pbuffer, vlength[0], v[0]); pbuffer += vlength[0];
  z_uint64_encode(pbuffer, vlength[1], v[1]); pbuffer += vlength[1];
  z_uint64_encode(pbuffer, vlength[2], v[2]); pbuffer += vlength[2];
  z_uint64_encode(pbuffer, vlength[3], v[3]); pbuffer += vlength[3];
  return(pbuffer - buffer);
}

/* ============================================================================
 *  list uint-unpacking
 */
void z_uint8_unpack4 (const uint8_t *buffer, uint8_t v[4]) {
  v[0] = buffer[2];
  v[1] = buffer[4];
  v[2] = buffer[6];
  v[3] = buffer[8];
}

int z_uint8_unpack4c (const uint8_t *buffer, uint32_t length, uint8_t v[4]) {
  int count = 0;
  switch (z_min(length, 9)) {
    case 9: v[3] = buffer[8]; count++;
    case 7: v[2] = buffer[6]; count++;
    case 5: v[1] = buffer[4]; count++;
    case 3: v[0] = buffer[2]; count++;
  }
  return(count);
}

void z_uint16_unpack4 (const uint8_t *buffer, uint16_t v[4]) {
  v[0] = buffer[1] << 8 | buffer[2];
  v[1] = buffer[3] << 8 | buffer[4];
  v[2] = buffer[5] << 8 | buffer[6];
  v[3] = buffer[7] << 8 | buffer[8];
}

int z_uint16_unpack4c (const uint8_t *buffer, uint32_t length, uint16_t v[4]) {
  int count = 0;
  switch (z_min(length, 9)) {
    case 9: v[3] = buffer[7] << 8 | buffer[8]; count++;
    case 7: v[2] = buffer[5] << 8 | buffer[6]; count++;
    case 5: v[1] = buffer[3] << 8 | buffer[4]; count++;
    case 3: v[0] = buffer[1] << 8 | buffer[2]; count++;
  }
  return(count);
}

void z_uint32_unpack4 (const uint8_t *buffer, uint32_t v[4]) {
  uint8_t vlength[4];

  vlength[0] = (1 + (buffer[0] & 3)) << 1;
  vlength[1] = (1 + ((buffer[0] >> 2) & 3)) << 1;
  vlength[2] = (1 + ((buffer[0] >> 4) & 3)) << 1;
  vlength[3] = (1 + ((buffer[0] >> 6) & 3)) << 1;

  z_uint32_decode(buffer + 1, vlength[0], v + 0); buffer += vlength[0] + 1;
  z_uint32_decode(buffer,     vlength[1], v + 1); buffer += vlength[1];
  z_uint32_decode(buffer,     vlength[2], v + 2); buffer += vlength[2];
  z_uint32_decode(buffer,     vlength[3], v + 3);
}

int z_uint32_unpack4c (const uint8_t *buffer, uint32_t length, uint32_t v[4]) {
  uint8_t vlength[4];

  vlength[0] = (1 + (buffer[0] & 3)) << 1;
  vlength[1] = (1 + ((buffer[0] >> 2) & 3)) << 1;
  vlength[2] = (1 + ((buffer[0] >> 4) & 3)) << 1;
  vlength[3] = (1 + ((buffer[0] >> 6) & 3)) << 1;

  z_uint32_decode(buffer + 1, vlength[0], v + 0);
  buffer += 1 + vlength[0]; length -= 1 + vlength[0];

  if (vlength[1] > length) return(1);
  z_uint32_decode(buffer, vlength[1], v + 1);
  buffer += vlength[1]; length -= vlength[1];

  if (vlength[2] > length) return(2);
  z_uint32_decode(buffer, vlength[2], v + 2);
  buffer += vlength[2]; length -= vlength[2];

  if (vlength[3] > length) return(3);
  z_uint32_decode(buffer, vlength[3], v + 3);
  return(4);
}

void z_uint64_unpack4 (const uint8_t *buffer, uint64_t v[4]) {
  uint8_t vlength[4];

  vlength[0] = (1 + (buffer[0] & 3)) << 1;
  vlength[1] = (1 + ((buffer[0] >> 2) & 3)) << 1;
  vlength[2] = (1 + ((buffer[0] >> 4) & 3)) << 1;
  vlength[3] = (1 + ((buffer[0] >> 6) & 3)) << 1;

  z_uint64_decode(buffer + 1, vlength[0], v + 0); buffer += vlength[0] + 1;
  z_uint64_decode(buffer,     vlength[1], v + 1); buffer += vlength[1];
  z_uint64_decode(buffer,     vlength[2], v + 2); buffer += vlength[2];
  z_uint64_decode(buffer,     vlength[3], v + 3);
}

int z_uint64_unpack4c (const uint8_t *buffer, uint32_t length, uint64_t v[4]) {
  uint8_t vlength[4];

  vlength[0] = (1 + (buffer[0] & 3)) << 1;
  vlength[1] = (1 + ((buffer[0] >> 2) & 3)) << 1;
  vlength[2] = (1 + ((buffer[0] >> 4) & 3)) << 1;
  vlength[3] = (1 + ((buffer[0] >> 6) & 3)) << 1;

  z_uint64_decode(buffer + 1, vlength[0], v + 0);
  buffer += 1 + vlength[0]; length -= 1 + vlength[0];

  if (vlength[1] > length) return(1);
  z_uint64_decode(buffer, vlength[1], v + 1);
  buffer += vlength[1]; length -= vlength[1];

  if (vlength[2] > length) return(2);
  z_uint64_decode(buffer, vlength[2], v + 2);
  buffer += vlength[2]; length -= vlength[2];

  if (vlength[3] > length) return(3);
  z_uint64_decode(buffer, vlength[3], v + 3);
  return(4);
}
