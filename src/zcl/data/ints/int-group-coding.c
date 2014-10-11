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
#include <zcl/bits.h>

/* ============================================================================
 *  Encode/Decode 4 unsigned integer of 32bit
 */
static int z_4int_decode_head (unsigned int head[4], uint8_t hbuf) {
  head[0] = 1 + z_2bit_fetch(hbuf, 6);
  head[1] = 1 + z_2bit_fetch(hbuf, 4);
  head[2] = 1 + z_2bit_fetch(hbuf, 2);
  head[3] = 1 + z_2bit_fetch(hbuf, 0);
  return(head[0] + head[1] + head[2] + head[3]);
}

int z_4int_encode (uint8_t *buf, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  unsigned int a_length = z_uint32_size(a);
  unsigned int b_length = z_uint32_size(b);
  unsigned int c_length = z_uint32_size(c);
  unsigned int d_length = z_uint32_size(d);
  uint8_t *pbuf = buf + 1;

  buf[0] = ((a_length - 1) << 6) | ((b_length - 1) << 4) |
           ((c_length - 1) << 2) | (d_length - 1);
  z_uint32_encode(pbuf, a_length, a); pbuf += a_length;
  z_uint32_encode(pbuf, b_length, b); pbuf += b_length;
  z_uint32_encode(pbuf, c_length, c); pbuf += c_length;
  z_uint32_encode(pbuf, d_length, d); pbuf += d_length;

  return(pbuf - buf);
}

int z_4int_decode (const uint8_t *buf,
                   uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
  unsigned int length[5];

  length[4] = z_4int_decode_head(length, *buf++);
  z_uint32_decode(buf, length[0], a); buf += length[0];
  z_uint32_decode(buf, length[1], b); buf += length[1];
  z_uint32_decode(buf, length[2], c); buf += length[2];
  z_uint32_decode(buf, length[3], d);

  return(1 + length[4]);
}

/* ============================================================================
 *  Encode/Decode 3 unsigned integer (2 uint64 and 1 uint32)
 */
static int z_3int_decode_head (unsigned int head[3], uint8_t hbuf) {
  head[0] = 1 + z_3bit_fetch(hbuf, 5);
  head[1] = 1 + z_3bit_fetch(hbuf, 2);
  head[2] = 1 + z_2bit_fetch(hbuf, 0);
  return(head[0] + head[1] + head[2]);
}

int z_3int_encode (uint8_t *buf, uint64_t a, uint64_t b, uint32_t c) {
  unsigned int a_length = z_uint64_size(a);
  unsigned int b_length = z_uint64_size(b);
  unsigned int c_length = z_uint32_size(c);
  uint8_t *pbuf = buf + 1;

  buf[0] = ((a_length - 1) << 5) | ((b_length - 1) << 2) | (c_length - 1);
  z_uint64_encode(pbuf, a_length, a); pbuf += a_length;
  z_uint64_encode(pbuf, b_length, b); pbuf += b_length;
  z_uint32_encode(pbuf, c_length, c); pbuf += c_length;

  return(pbuf - buf);
}

int z_3int_decode (const uint8_t *buf, uint64_t *a, uint64_t *b, uint32_t *c) {
  unsigned int length[4];

  length[3] = z_3int_decode_head(length, *buf++);
  z_uint64_decode(buf, length[0], a); buf += length[0];
  z_uint64_decode(buf, length[1], b); buf += length[1];
  z_uint32_decode(buf, length[2], c);

  return(1 + length[3]);
}
