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

#include <zcl/field-coding.h>
#include <zcl/int-coding.h>

int z_field_write_uint8 (uint8_t *buf, uint16_t field_id, uint8_t value) {
  int len;

  len = z_field_write(buf, field_id, 1);
  buf[len++] = value;

  return(len);
}

int z_field_write_uint16 (uint8_t *buf, uint16_t field_id, uint16_t value) {
  int vlength;
  int len;

  /* write the field-id with vlength */
  vlength = z_uint16_size(value);
  len = z_field_write(buf, field_id, vlength);

  /* write the value */
  z_uint16_encode(buf + len, vlength, value);
  return(len + vlength);
}

int z_field_write_uint32 (uint8_t *buf, uint16_t field_id, uint32_t value) {
  int vlength;
  int len;

  /* write the field-id */
  vlength = z_uint32_size(value);
  len = z_field_write(buf, field_id, vlength);

  /* write the value */
  z_uint32_encode(buf + len, vlength, value);
  return(len + vlength);
}

int z_field_write_uint64 (uint8_t *buf, uint16_t field_id, uint64_t value) {
  int vlength;
  int len;

  /* write the field-id with vlength */
  vlength = z_uint64_size(value);
  len = z_field_write(buf, field_id, vlength);

  /* write the value */
  z_uint64_encode(buf + len, vlength, value);
  return(len + vlength);
}

int z_field_write (uint8_t *buf, uint16_t field_id, int vlength) {
  uint8_t *pbuf = buf;
  int len;

  if (field_id < 30) {
    *pbuf = (field_id << 3) | (vlength - 1);
    len = 1;
  } else if (field_id > 285) {
    field_id -= 31;
    *pbuf++ = (31 << 3) | (vlength - 1);
    *pbuf++ = field_id >> 8;
    *pbuf = field_id & 0xff;
    len = 3;
  } else {
    field_id -= 30;
    *pbuf++ = (30 << 3) | (vlength - 1);
    *pbuf++ = field_id & 0xff;
    len = 2;
  }
  return(len);
}
