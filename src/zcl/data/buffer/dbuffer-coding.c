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
#include <zcl/dbuffer.h>
#include <zcl/string.h>
#include <zcl/debug.h>

/* ===========================================================================
 *  Field Writer Helper
 */
uint8_t *z_dbuf_write_field_mark (z_dbuf_writer_t *self,
                                  uint16_t field_id,
                                  int vlength)
{
  uint8_t buffer[4];
  uint8_t *pbuf;
  int len;

  pbuf = z_dbuf_writer_get(self, buffer, 4);
  len = z_field_write(pbuf, field_id, vlength);
  z_dbuf_writer_commit(self, pbuf, len);

  return(z_dbuf_writer_mark(self, vlength));
}

int z_dbuf_write_field_u8 (z_dbuf_writer_t *self,
                           uint16_t field_id,
                           uint8_t value)
{
  uint8_t buffer[4];
  uint8_t *pbuf;
  int len;

  pbuf = z_dbuf_writer_get(self, buffer, 4);
  len = z_field_write_uint8(pbuf, field_id, value);
  z_dbuf_writer_commit(self, pbuf, len);

  return(len);
}

int z_dbuf_write_field_u16 (z_dbuf_writer_t *self,
                            uint16_t field_id,
                            uint16_t value)
{
  uint8_t buffer[5];
  uint8_t *pbuf;
  int len;

  pbuf = z_dbuf_writer_get(self, buffer, 5);
  len = z_field_write_uint16(pbuf, field_id, value);
  z_dbuf_writer_commit(self, pbuf, len);

  return(len);
}

int z_dbuf_write_field_u32 (z_dbuf_writer_t *self,
                            uint16_t field_id,
                            uint32_t value)
{
  uint8_t buffer[7];
  uint8_t *pbuf;
  int len;

  pbuf = z_dbuf_writer_get(self, buffer, 7);
  len = z_field_write_uint32(pbuf, field_id, value);
  z_dbuf_writer_commit(self, pbuf, len);

  return(len);
}

int z_dbuf_write_field_u64 (z_dbuf_writer_t *self,
                            uint16_t field_id,
                            uint64_t value)
{
  uint8_t buffer[11];
  uint8_t *pbuf;
  int len;

  pbuf = z_dbuf_writer_get(self, buffer, 11);
  len = z_field_write_uint64(pbuf, field_id, value);
  z_dbuf_writer_commit(self, pbuf, len);

  return(len);
}

/* ===========================================================================
 *  Numeric Writer Helper
 */
uint16_t z_dbuf_write_vint32 (z_dbuf_writer_t *self, uint32_t value) {
  uint8_t buffer[5];
  uint8_t *phead;
  uint8_t *pbuf;
  uint16_t len;

  pbuf = phead = z_dbuf_writer_get(self, buffer, 5);
  len = z_vint32_encode(pbuf, value) - phead;
  z_dbuf_writer_commit(self, pbuf, len);

  return(len);
}

uint16_t z_dbuf_write_vint64 (z_dbuf_writer_t *self, uint64_t value) {
  uint8_t buffer[9];
  uint8_t *phead;
  uint8_t *pbuf;
  uint16_t len;

  pbuf = phead = z_dbuf_writer_get(self, buffer, 9);
  len = z_vint64_encode(pbuf, value) - phead;
  z_dbuf_writer_commit(self, pbuf, len);

  return(len);
}

/* ===========================================================================
 *  List Writer Helper
 */
uint16_t z_dbuf_write_u8_list (z_dbuf_writer_t *writer,
                               const uint8_t *v, int count)
{
  uint8_t buffer[9];
  uint16_t size = 0;
  int len;

  while (count >= 4) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(writer, buffer, 9);
    len = z_uint8_pack4(pbuf, v);
    z_dbuf_writer_commit(writer, pbuf, len);

    size += len;
    v += 4;
    count -= 4;
  }

  if (count > 0) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(writer, buffer, 1 + (count << 1));
    switch (count) {
      case 3: len = z_uint8_pack3(pbuf,  v); break;
      case 2: len = z_uint8_pack2(pbuf,  v); break;
      case 1: len = z_uint8_pack1(pbuf, *v); break;
    }
    z_dbuf_writer_commit(writer, pbuf, len);
    size += len;
  }
  return(size);
}

uint16_t z_dbuf_write_u16_list (z_dbuf_writer_t *writer,
                                const uint16_t *v, int count)
{
  uint8_t buffer[9];
  uint16_t size = 0;
  int len;

  while (count >= 4) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(writer, buffer, 9);
    len = z_uint16_pack4(pbuf, v);
    z_dbuf_writer_commit(writer, pbuf, len);

    size += len;
    v += 4;
    count -= 4;
  }

  if (count > 0) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(writer, buffer, 1 + (count << 1));
    switch (count) {
      case 3: len = z_uint16_pack3(pbuf,  v); break;
      case 2: len = z_uint16_pack2(pbuf,  v); break;
      case 1: len = z_uint16_pack1(pbuf, *v); break;
    }
    z_dbuf_writer_commit(writer, pbuf, len);
    size += len;
  }
  return(size);
}

uint16_t z_dbuf_write_u32_list (z_dbuf_writer_t *writer,
                                const uint32_t *v, int count)
{
  uint8_t buffer[17];
  uint16_t size = 0;
  int len;

  while (count >= 4) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(writer, buffer, 17);
    len = z_uint32_pack4(pbuf, v);
    z_dbuf_writer_commit(writer, pbuf, len);

    size += len;
    v += 4;
    count -= 4;
  }

  if (count > 0) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(writer, buffer, 1 + (count << 2));
    switch (count) {
      case 3: len = z_uint32_pack3(pbuf,  v); break;
      case 2: len = z_uint32_pack2(pbuf,  v); break;
      case 1: len = z_uint32_pack1(pbuf, *v); break;
    }
    z_dbuf_writer_commit(writer, pbuf, len);
    size += len;
  }
  return(size);
}

uint16_t z_dbuf_write_u64_list (z_dbuf_writer_t *writer,
                                const uint64_t *v, int count)
{
  uint8_t buffer[33];
  uint16_t size = 0;
  int len;

  while (count >= 4) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(writer, buffer, 33);
    len = z_uint64_pack4(pbuf, v);
    z_dbuf_writer_commit(writer, pbuf, len);

    size += len;
    v += 4;
    count -= 4;
  }

  if (count > 0) {
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(writer, buffer, 1 + (count << 3));
    switch (count) {
      case 3: len = z_uint64_pack3(pbuf,  v); break;
      case 2: len = z_uint64_pack2(pbuf,  v); break;
      case 1: len = z_uint64_pack1(pbuf, *v); break;
    }
    z_dbuf_writer_commit(writer, pbuf, len);
    size += len;
  }
  return(size);
}

uint16_t z_dbuf_write_histogram (z_dbuf_writer_t *self,
                                 const z_histogram_t *histo)
{
  const uint64_t *pbound;
  const uint64_t *pevent;
  uint64_t values[4];
  uint16_t size = 0;
  int h, len;

  pbound = histo->bounds;
  pevent = histo->events;
  for (h = 0; histo->max > *pbound; pbound++, pevent++) {
    if (*pevent > 0) {
      values[h++] = *pbound;
      values[h++] = *pevent;
    }
    if (h == 4) {
      uint8_t buffer[36];
      uint8_t *pbuf;
      pbuf = z_dbuf_writer_get(self, buffer, 33);
      len = z_uint64_pack4(pbuf, values);
      z_dbuf_writer_commit(self, pbuf, len);
      size += len;
      h = 0;
    }
  }

  if (*pevent > 0) {
    values[h++] = histo->max;
    values[h++] = *pevent;
  }

  if (h > 0) {
    uint8_t buffer[36];
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_get(self, buffer, 1 + (h << 3));
    switch (h) {
      case 4: len = z_uint64_pack4(pbuf, values);  break;
      case 3: len = z_uint64_pack3(pbuf, values);  break;
      case 2: len = z_uint64_pack2(pbuf, values);  break;
      case 1: len = z_uint64_pack1(pbuf, *values); break;
    }
    z_dbuf_writer_commit(self, pbuf, len);
    size += len;
  }
  return(size);
}