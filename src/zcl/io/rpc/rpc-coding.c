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
#include <zcl/rpc.h>

int z_rpc_write_uint8 (uint8_t *buf, uint16_t field_id, uint8_t value) {
  uint8_t *pbuf = buf;

  if (field_id < 30) {
    *pbuf++ = (field_id << 3);
  } else if (field_id > 285) {
    field_id -= 31;
    *pbuf++ = (31 << 3);
    *pbuf++ = field_id >> 8;
    *pbuf++ = field_id & 0xff;
  } else {
    field_id -= 30;
    *pbuf++ = (30 << 3);
    *pbuf++ = field_id & 0xff;
  }
  *pbuf++ = value;

  return(pbuf - buf);
}

int z_rpc_write_uint16 (uint8_t *buf, uint16_t field_id, uint16_t value) {
  uint8_t *pbuf = buf;

  /* write the field-id with vlength 0 */
  if (field_id < 30) {
    *pbuf++ = (field_id << 3);
  } else if (field_id > 285) {
    field_id -= 31;
    *pbuf++ = (31 << 3);
    *pbuf++ = field_id >> 8;
    *pbuf++ = field_id & 0xff;
  } else {
    field_id -= 30;
    *pbuf++ = (30 << 3);
    *pbuf++ = field_id & 0xff;
  }

  if (value < (1ull << 8)) {
    *pbuf++ = value & 0xff;
  } else {
    buf[0] |= 1;
    *pbuf++ = value & 0xff;
    *pbuf++ = (value >> 8) & 0xff;
  }
  return(pbuf - buf);
}

int z_rpc_write_uint32 (uint8_t *buf, uint16_t field_id, uint32_t value) {
  uint8_t *pbuf = buf;
  int vlength;

  /* write the field-id */
  vlength = z_uint32_size(value);
  if (field_id < 30) {
    *pbuf++ = (field_id << 3) | (vlength - 1);
  } else if (field_id > 285) {
    field_id -= 31;
    *pbuf++ = (31 << 3) | (vlength - 1);
    *pbuf++ = field_id >> 8;
    *pbuf++ = field_id & 0xff;
  } else {
    field_id -= 30;
    *pbuf++ = (30 << 3) | (vlength - 1);
    *pbuf++ = field_id & 0xff;
  }

  /* write the value */
  z_uint32_encode(pbuf, vlength, value);
  return((pbuf - buf) + vlength);
}

int z_rpc_write_uint64 (uint8_t *buf, uint16_t field_id, uint64_t value) {
  uint8_t *pbuf = buf;
  int vlength;

  /* write the field-id with vlength 0 */
  vlength = z_uint64_size(value);
  if (field_id < 30) {
    *pbuf++ = (field_id << 3) | (vlength - 1);
  } else if (field_id > 285) {
    field_id -= 31;
    *pbuf++ = (31 << 3) | (vlength - 1);
    *pbuf++ = field_id >> 8;
    *pbuf++ = field_id & 0xff;
  } else {
    field_id -= 30;
    *pbuf++ = (30 << 3) | (vlength - 1);
    *pbuf++ = field_id & 0xff;
  }

  /* write the value */
  z_uint64_encode(pbuf, vlength, value);
  return((pbuf - buf) + vlength);
}

/* ============================================================================
 *  dbuf helpers
 */
uint8_t *z_rpc_write_mark8 (z_dbuf_writer_t *writer, uint16_t field_id) {
  uint8_t buffer[5];
  uint8_t *pbuf;
  int len;
  pbuf = z_dbuf_writer_next(writer, buffer, 5);
  len  = z_rpc_write_uint8(pbuf, field_id, 0xff);
  pbuf = z_dbuf_writer_commit(writer, pbuf, len);
  return(pbuf + len - 1);
}

uint16_t *z_rpc_write_mark16 (z_dbuf_writer_t *writer, uint16_t field_id) {
  uint8_t buffer[8];
  uint8_t *pbuf;
  int len;
  pbuf = z_dbuf_writer_next(writer, buffer, 8);
  len  = z_rpc_write_uint16(pbuf, field_id, 0xffff);
  pbuf = z_dbuf_writer_commit(writer, pbuf, len);
  return((uint16_t *)(pbuf + len - 2));
}

uint16_t z_rpc_write_field_count (z_dbuf_writer_t *writer,
                                  uint8_t field_count)
{
  uint8_t buffer[1];
  uint8_t *pbuf;
  pbuf = z_dbuf_writer_next(writer, buffer, 1);
  pbuf[0] = field_count;
  z_dbuf_writer_commit(writer, pbuf, 1);
  return(1);
}

uint16_t z_rpc_write_u8_list (z_dbuf_writer_t *writer,
                              const uint8_t *v, int count)
{
  uint8_t buffer[9];
  uint16_t size = 0;
  int len;

  while (count >= 4) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(writer, buffer, 9);
    len = z_rpc_uint8_pack4(pbuf, v);
    z_dbuf_writer_commit(writer, pbuf, len);

    size += len;
    v += 4;
    count -= 4;
  }

  if (count > 0) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(writer, buffer, 1 + (count << 1));
    switch (count) {
      case 3: len = z_rpc_uint8_pack3(pbuf,  v); break;
      case 2: len = z_rpc_uint8_pack2(pbuf,  v); break;
      case 1: len = z_rpc_uint8_pack1(pbuf, *v); break;
    }
    z_dbuf_writer_commit(writer, pbuf, len);
    size += len;
  }
  return(size);
}


uint16_t z_rpc_write_u16_list (z_dbuf_writer_t *writer,
                               const uint16_t *v, int count)
{
  uint8_t buffer[9];
  uint16_t size = 0;
  int len;

  while (count >= 4) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(writer, buffer, 9);
    len = z_rpc_uint16_pack4(pbuf, v);
    z_dbuf_writer_commit(writer, pbuf, len);

    size += len;
    v += 4;
    count -= 4;
  }

  if (count > 0) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(writer, buffer, 1 + (count << 1));
    switch (count) {
      case 3: len = z_rpc_uint16_pack3(pbuf,  v); break;
      case 2: len = z_rpc_uint16_pack2(pbuf,  v); break;
      case 1: len = z_rpc_uint16_pack1(pbuf, *v); break;
    }
    z_dbuf_writer_commit(writer, pbuf, len);
    size += len;
  }
  return(size);
}

uint16_t z_rpc_write_u32_list (z_dbuf_writer_t *writer,
                               const uint32_t *v, int count)
{
  uint8_t buffer[17];
  uint16_t size = 0;
  int len;

  while (count >= 4) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(writer, buffer, 17);
    len = z_rpc_uint32_pack4(pbuf, v);
    z_dbuf_writer_commit(writer, pbuf, len);

    size += len;
    v += 4;
    count -= 4;
  }

  if (count > 0) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(writer, buffer, 1 + (count << 2));
    switch (count) {
      case 3: len = z_rpc_uint32_pack3(pbuf,  v); break;
      case 2: len = z_rpc_uint32_pack2(pbuf,  v); break;
      case 1: len = z_rpc_uint32_pack1(pbuf, *v); break;
    }
    z_dbuf_writer_commit(writer, pbuf, len);
    size += len;
  }
  return(size);
}

uint16_t z_rpc_write_u64_list (z_dbuf_writer_t *writer,
                               const uint64_t *v, int count)
{
  uint8_t buffer[33];
  uint16_t size = 0;
  int len;

  while (count >= 4) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(writer, buffer, 33);
    len = z_rpc_uint64_pack4(pbuf, v);
    z_dbuf_writer_commit(writer, pbuf, len);

    size += len;
    v += 4;
    count -= 4;
  }

  if (count > 0) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(writer, buffer, 1 + (count << 3));
    switch (count) {
      case 3: len = z_rpc_uint64_pack3(pbuf,  v); break;
      case 2: len = z_rpc_uint64_pack2(pbuf,  v); break;
      case 1: len = z_rpc_uint64_pack1(pbuf, *v); break;
    }
    z_dbuf_writer_commit(writer, pbuf, len);
    size += len;
  }
  return(size);
}

/* ============================================================================
 *  list uint8-packing
 */
int z_rpc_uint8_pack1 (uint8_t *buffer, uint8_t v) {
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = v;
  return(3);
}

int z_rpc_uint8_pack2 (uint8_t *buffer, const uint8_t v[2]) {
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = v[0];
  buffer[3] = 0;
  buffer[4] = v[1];
  return(5);
}

int z_rpc_uint8_pack3 (uint8_t *buffer, const uint8_t v[3]) {
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = v[0];
  buffer[3] = 0;
  buffer[4] = v[1];
  buffer[5] = 0;
  buffer[6] = v[2];
  return(7);
}

int z_rpc_uint8_pack4 (uint8_t *buffer, const uint8_t v[4]) {
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
int z_rpc_uint16_pack1 (uint8_t *buffer, uint16_t v) {
  buffer[0] = 0;
  buffer[1] = (v >> 8);
  buffer[2] = (v & 0xff);
  return(3);
}

int z_rpc_uint16_pack2 (uint8_t *buffer, const uint16_t v[2]) {
  buffer[0] = 0;
  buffer[1] = (v[0] >> 8);
  buffer[2] = (v[0] & 0xff);
  buffer[3] = (v[1] >> 8);
  buffer[4] = (v[1] & 0xff);
  return(5);
}

int z_rpc_uint16_pack3 (uint8_t *buffer, const uint16_t v[3]) {
  buffer[0] = 0;
  buffer[1] = (v[0] >> 8);
  buffer[2] = (v[0] & 0xff);
  buffer[3] = (v[1] >> 8);
  buffer[4] = (v[1] & 0xff);
  buffer[5] = (v[2] >> 8);
  buffer[6] = (v[2] & 0xff);
  return(7);
}

int z_rpc_uint16_pack4 (uint8_t *buffer, const uint16_t v[4]) {
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
int z_rpc_uint32_pack1 (uint8_t *buffer, uint32_t v) {
  uint8_t vlength;

  vlength = z_uint32_size(v);
  vlength = z_align_up(vlength, 2);

  *buffer++ = (vlength >> 1) - 1;
  z_uint32_encode(buffer, vlength, v);
  return(vlength + 1);
}

int z_rpc_uint32_pack2 (uint8_t *buffer, const uint32_t v[2]) {
  uint8_t *pbuffer = buffer;
  uint8_t vlength[2];

  vlength[0] = z_uint32_size(v[0]); vlength[0] = z_align_up(vlength[0], 2);
  vlength[1] = z_uint32_size(v[1]); vlength[1] = z_align_up(vlength[1], 2);

  *pbuffer++ = ((vlength[1] >> 1) - 1) << 2 | ((vlength[0] >> 1) - 1);
  z_uint32_encode(pbuffer, vlength[0], v[0]); pbuffer += vlength[0];
  z_uint32_encode(pbuffer, vlength[1], v[1]); pbuffer += vlength[1];
  return(pbuffer - buffer);
}

int z_rpc_uint32_pack3 (uint8_t *buffer, const uint32_t v[3]) {
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

int z_rpc_uint32_pack4 (uint8_t *buffer, const uint32_t v[4]) {
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
int z_rpc_uint64_pack1 (uint8_t *buffer, uint64_t v) {
  uint8_t vlength;

  vlength = z_uint64_size(v);
  vlength = z_align_up(vlength, 2);

  *buffer++ = (vlength >> 1) - 1;
  z_uint64_encode(buffer, vlength, v);
  return(vlength + 1);
}

int z_rpc_uint64_pack2 (uint8_t *buffer, const uint64_t v[2]) {
  uint8_t *pbuffer = buffer;
  uint8_t vlength[2];

  vlength[0] = z_uint64_size(v[0]); vlength[0] = z_align_up(vlength[0], 2);
  vlength[1] = z_uint64_size(v[1]); vlength[1] = z_align_up(vlength[1], 2);

  *pbuffer++ = ((vlength[1] >> 1) - 1) << 2 | ((vlength[0] >> 1) - 1);
  z_uint64_encode(pbuffer, vlength[0], v[0]); pbuffer += vlength[0];
  z_uint64_encode(pbuffer, vlength[1], v[1]); pbuffer += vlength[1];
  return(pbuffer - buffer);
}

int z_rpc_uint64_pack3 (uint8_t *buffer, const uint64_t v[3]) {
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

int z_rpc_uint64_pack4 (uint8_t *buffer, const uint64_t v[4]) {
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
void z_rpc_uint8_unpack4 (const uint8_t *buffer, uint8_t v[4]) {
  v[0] = buffer[2];
  v[1] = buffer[4];
  v[2] = buffer[6];
  v[3] = buffer[8];
}

int z_rpc_uint8_unpack4c (const uint8_t *buffer, uint32_t length, uint8_t v[4]) {
  int count = 0;
  switch (z_min(length, 9)) {
    case 9: v[3] = buffer[8]; count++;
    case 7: v[2] = buffer[6]; count++;
    case 5: v[1] = buffer[4]; count++;
    case 3: v[0] = buffer[2]; count++;
  }
  return(count);
}

void z_rpc_uint16_unpack4 (const uint8_t *buffer, uint16_t v[4]) {
  v[0] = buffer[1] << 8 | buffer[2];
  v[1] = buffer[3] << 8 | buffer[4];
  v[2] = buffer[5] << 8 | buffer[6];
  v[3] = buffer[7] << 8 | buffer[8];
}

int z_rpc_uint16_unpack4c (const uint8_t *buffer, uint32_t length, uint16_t v[4]) {
  int count = 0;
  switch (z_min(length, 9)) {
    case 9: v[3] = buffer[7] << 8 | buffer[8]; count++;
    case 7: v[2] = buffer[5] << 8 | buffer[6]; count++;
    case 5: v[1] = buffer[3] << 8 | buffer[4]; count++;
    case 3: v[0] = buffer[1] << 8 | buffer[2]; count++;
  }
  return(count);
}

void z_rpc_uint32_unpack4 (const uint8_t *buffer, uint32_t v[4]) {
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

int z_rpc_uint32_unpack4c (const uint8_t *buffer, uint32_t length, uint32_t v[4]) {
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

void z_rpc_uint64_unpack4 (const uint8_t *buffer, uint64_t v[4]) {
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

int z_rpc_uint64_unpack4c (const uint8_t *buffer, uint32_t length, uint64_t v[4]) {
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

/* ============================================================================
 *  Field Reader
 */
int z_rpc_iter_field_uint8 (z_rpc_field_iter_t *self,
                            uint16_t field_id,
                            uint8_t *value)
{
  const uint8_t *block = self->pbuffer;
  int flen, fid;

  while (self->remaining--) {
    flen = 1 + (*block & 7);
    fid = *block++ >> 3;
    switch (fid) {
      case 31: fid += *block++ << 8;
      case 30: fid += *block++;
    }

    if (field_id == fid)
      goto _read_u8;
    if (field_id < fid)
      break;

    block += flen;
  }
  self->pbuffer = block + flen;
  return(-1);

_read_u8:
  self->pbuffer = block + flen;
  if (flen == 1) {
    *value = block[0];
    return(0);
  }
  return(-1);
}

int z_rpc_iter_field_uint16  (z_rpc_field_iter_t *self,
                             uint16_t field_id,
                             uint16_t *value)
{
  const uint8_t *block = self->pbuffer;
  int flen, fid;

  while (self->remaining--) {
    flen = 1 + (*block & 7);
    fid = *block++ >> 3;
    switch (fid) {
      case 31: fid += *block++ << 8;
      case 30: fid += *block++;
    }

    if (field_id == fid)
      goto _read_u16;
    if (field_id < fid)
      break;

    block += flen;
  }
  self->pbuffer = block + flen;
  return(-1);

_read_u16:
  *value = 0;
  self->pbuffer = block + flen;
  switch (flen) {
    case 2: *value |= block[1] <<  8;
    case 1: *value |= block[0];
            return(0);
  }
  return(-1);
}

int z_rpc_iter_field_uint32 (z_rpc_field_iter_t *self,
                             uint16_t field_id,
                             uint32_t *value)
{
  const uint8_t *block = self->pbuffer;
  int flen, fid;

  while (self->remaining--) {
    flen = 1 + (*block & 7);
    fid = *block++ >> 3;
    switch (fid) {
      case 31: fid += *block++ << 8;
      case 30: fid += *block++;
    }

    if (field_id == fid)
      goto _read_u32;
    if (field_id < fid)
      break;

    block += flen;
  }
  self->pbuffer = block + flen;
  return(-1);

_read_u32:
  *value = 0;
  self->pbuffer = block + flen;
  switch (flen) {
    case 4: *value |= z_shl24(block[3]);
    case 3: *value |= block[2] << 16;
    case 2: *value |= block[1] <<  8;
    case 1: *value |= block[0];
            return(0);
  }
  return(-1);
}

int z_rpc_iter_field_uint64 (z_rpc_field_iter_t *self,
                             uint16_t field_id,
                             uint64_t *value)
{
  const uint8_t *block = self->pbuffer;
  int flen, fid;

  while (self->remaining--) {
    flen = 1 + (*block & 7);
    fid = *block++ >> 3;
    switch (fid) {
      case 31: fid += *block++ << 8;
      case 30: fid += *block++;
    }

    if (field_id == fid)
      goto _read_u64;
    if (field_id < fid)
      break;

    block += flen;
  }
  self->pbuffer = block + flen;
  return(-1);

_read_u64:
  *value = 0;
  self->pbuffer = block + flen;
  z_uint64_decode(block, flen, value);
  switch (flen) {
    case 8: *value |= z_shl56(block[7]);
    case 7: *value |= z_shl48(block[6]);
    case 6: *value |= z_shl40(block[5]);
    case 5: *value |= z_shl32(block[4]);
    case 4: *value |= z_shl24(block[3]);
    case 3: *value |= block[2] << 16;
    case 2: *value |= block[1] <<  8;
    case 1: *value |= block[0];
            return(0);
  }
  return(-1);
}
