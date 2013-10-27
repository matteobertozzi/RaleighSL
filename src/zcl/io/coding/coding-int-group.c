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

#include <zcl/coding.h>

/* ============================================================================
 *  Encode/Decode 4 unsigned integer of 32bit
 */
static int z_decode_4int_head (unsigned int head[4], uint8_t hbuf) {
  head[0] = 1 + z_fetch_2bit(hbuf, 6);
  head[1] = 1 + z_fetch_2bit(hbuf, 4);
  head[2] = 1 + z_fetch_2bit(hbuf, 2);
  head[3] = 1 + z_fetch_2bit(hbuf, 0);
  return(head[0] + head[1] + head[2] + head[3]);
}

int z_encode_4int (uint8_t *buf, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  unsigned int a_length = z_uint32_size(a);
  unsigned int b_length = z_uint32_size(b);
  unsigned int c_length = z_uint32_size(c);
  unsigned int d_length = z_uint32_size(d);
  uint8_t *pbuf = buf + 1;

  buf[0] = ((a_length - 1) << 6) | ((b_length - 1) << 4) |
           ((c_length - 1) << 2) | (d_length - 1);
  z_encode_uint(pbuf, a_length, a); pbuf += a_length;
  z_encode_uint(pbuf, b_length, b); pbuf += b_length;
  z_encode_uint(pbuf, c_length, c); pbuf += c_length;
  z_encode_uint(pbuf, d_length, d); pbuf += d_length;

  return(pbuf - buf);
}

int z_decode_4int (const uint8_t *buf,
                   uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
  unsigned int length[5];

  length[4] = z_decode_4int_head(length, *buf++);
  z_decode_uint32(buf, length[0], a); buf += length[0];
  z_decode_uint32(buf, length[1], b); buf += length[1];
  z_decode_uint32(buf, length[2], c); buf += length[2];
  z_decode_uint32(buf, length[3], d);

  return(1 + length[4]);
}

/* ============================================================================
 *  Encode/Decode 3 unsigned integer (2 uint64 and 1 uint32)
 */
static int z_decode_3int_head (unsigned int head[3], uint8_t hbuf) {
  head[0] = 1 + z_fetch_3bit(hbuf, 5);
  head[1] = 1 + z_fetch_3bit(hbuf, 2);
  head[2] = 1 + z_fetch_2bit(hbuf, 0);
  return(head[0] + head[1] + head[2]);
}

int z_encode_3int (uint8_t *buf, uint64_t a, uint64_t b, uint32_t c) {
  unsigned int a_length = z_uint64_size(a);
  unsigned int b_length = z_uint64_size(b);
  unsigned int c_length = z_uint32_size(c);
  uint8_t *pbuf = buf + 1;

  buf[0] = ((a_length - 1) << 5) | ((b_length - 1) << 2) | (c_length - 1);
  z_encode_uint(pbuf, a_length, a); pbuf += a_length;
  z_encode_uint(pbuf, b_length, b); pbuf += b_length;
  z_encode_uint(pbuf, c_length, c); pbuf += c_length;

  return(pbuf - buf);
}

int z_decode_3int (const uint8_t *buf, uint64_t *a, uint64_t *b, uint32_t *c) {
  unsigned int length[4];

  length[3] = z_decode_3int_head(length, *buf++);
  z_decode_uint64(buf, length[0], a); buf += length[0];
  z_decode_uint64(buf, length[1], b); buf += length[1];
  z_decode_uint32(buf, length[2], c);

  return(1 + length[3]);
}
