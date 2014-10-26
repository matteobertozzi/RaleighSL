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

/*
 * Field encoding
 * ===============================================
 * A field is identified by an 16bit Id and has a 64bit length.
 * For RPC usage, most common fields are uint, so between 1 and 8 bytes.
 * and a message contains in general few fields (less then 12).
 *
 * +-+---+----+
 * |0|111|1111|
 * +-+---+----+
 *    |   +--------> Field Id (0-13)
 *    +------------> Has External Length
 *
 * Given that assumption, we can use one single byte to encode a field with
 * an Id between 0 and 13 and a length between 1 and 8 bytes.
 *
 * +-+---+----+ +----------+
 * |0|111|1111| | Field Id |
 * +-+---+----+ +----------+
 *  | |   +--------> Field Id (0-13)
 *  | +------------> Field Length
 *  +--------------> Has External Length
 *
 * The first bit is used to know if the field length can fit or not 3bits.
 * If it's not set, the field length is less or equals than 8 bytes.
 * The Field-Id can be up to 16bit, so we reserve 0 and 1 value and
 * if the field-id less then 13 we add 2, and if it's over we store the length
 * that can be 0 or 1.
 *
 * +-+---+----+ +--------------+ +----------+
 * |1|111|1111| | Field Length | | Field Id |
 * +-+---+----+ +--------------+ +----------+
 *  | |   +--------> Field Id (0-13)
 *  | +------------> External Length
 *  +--------------> Has External Length
 *
 * If the first bit is set the length is stored outside the head byte and
 * the length is stored in the 3bits.
 */
int z_field_encode (uint8_t *buf, uint16_t field_id, uint64_t length) {
  unsigned int elength = 1;

  if (length <= 8) {
    /* Internal Length (uint8-uint64) */
    buf[0] = ((length - 1) & 0x7) << 4;
  } else {
    /* External Length */
    unsigned int flen = z_uint64_size(length);
    buf[0] = (1 << 7) | (((flen - 1) & 0x7) << 4);
    z_uint_encode(buf + 1, flen, length);
    elength += flen;
  }

  if (field_id <= 13) {
    /* Field-Id can stay in the head */
    buf[0] |= field_id + 2;
  } else {
    /* Field-Id is to high, store Field-Id length here (max 2 byte) */
    unsigned int flen = z_uint32_size(field_id);
    buf[0] |= (flen - 1);
    z_uint_encode(buf + elength, flen, field_id);
    elength += flen;
  }

  return(elength);
}

uint8_t z_field_encoded_length (uint16_t field_id, uint64_t length) {
  uint8_t elength = 1;
  if (length > 8)
    elength += z_uint64_size(length);
  if (field_id > 13)
    elength += z_uint32_size(field_id);
  return(elength);
}

int z_field_decode (const uint8_t *buf,
                    unsigned int buflen,
                    uint16_t *field_id,
                    uint64_t *length)
{
  unsigned int elength = 1;
  unsigned int flen0;
  unsigned int flen1;

  flen0 = 1 + z_3bit_fetch(buf[0], 4);
  flen1 = z_4bit_fetch(buf[0], 0);

  if (buf[0] & (1 << 7)) {
    /* External Length */
    elength += flen0;
    if (Z_UNLIKELY(buflen < elength))
        return(-(elength + (flen1 >= 2 ? 0 : (flen1 + 1))));
    z_uint64_decode(buf + 1, flen0, length);
  } else {
    /* Internal Length (uint8-uint64) */
    *length = flen0;
  }

  if (flen1 >= 2) {
    /* Field-Id is in the head */
    *field_id = flen1 - 2;
  } else {
    /* Field-Id is stored outside */
    flen1 += 1;
    if (Z_UNLIKELY(buflen < (elength + flen1)))
        return(-(elength + flen1));
    z_uint16_decode(buf + elength, flen1, field_id);
    elength += flen1;
  }

  return(elength);
}
