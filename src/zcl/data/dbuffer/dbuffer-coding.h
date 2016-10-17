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

#ifndef _Z_DBUFFER_CODING_H_
#define _Z_DBUFFER_CODING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/histogram.h>
#include <zcl/dbuffer.h>
#include <zcl/macros.h>

#define z_dbuf_write_field_mark_type(self, field_id, type)              \
  Z_CAST(type, z_dbuf_write_field_mark(self, field_id, sizeof(type)))

#define z_dbuf_write_field_mark_u16(self, field_id)                     \
  z_dbuf_write_field_mark_type(self, field_id, uint16_t)

#define z_dbuf_write_field_mark_u32(self, field_id)                     \
  z_dbuf_write_field_mark_type(self, field_id, uint32_t)

#define z_dbuf_write_field_mark_u64(self, field_id)                     \
  z_dbuf_write_field_mark_type(self, field_id, uint64_t)

uint8_t *z_dbuf_write_field_mark (z_dbuf_writer_t *self,
                                  uint16_t field_id,
                                  int vlength);

uint16_t z_dbuf_write_field_u8   (z_dbuf_writer_t *self,
                                  uint16_t field_id,
                                  uint8_t value);
uint16_t z_dbuf_write_field_u16  (z_dbuf_writer_t *self,
                                  uint16_t field_id,
                                  uint16_t value);
uint16_t z_dbuf_write_field_u32  (z_dbuf_writer_t *self,
                                  uint16_t field_id,
                                  uint32_t value);
uint16_t z_dbuf_write_field_u64  (z_dbuf_writer_t *self,
                                  uint16_t field_id,
                                  uint64_t value);

uint16_t z_dbuf_write_vint32     (z_dbuf_writer_t *self, uint32_t value);
uint16_t z_dbuf_write_vint64     (z_dbuf_writer_t *self, uint64_t value);

uint16_t z_dbuf_write_u8_list    (z_dbuf_writer_t *writer,
                                  const uint8_t *v, int count);
uint16_t z_dbuf_write_u16_list   (z_dbuf_writer_t *writer,
                                  const uint16_t *v, int count);
uint16_t z_dbuf_write_u32_list   (z_dbuf_writer_t *writer,
                                  const uint32_t *v, int count);
uint16_t z_dbuf_write_u64_list   (z_dbuf_writer_t *writer,
                                  const uint64_t *v, int count);

uint16_t z_dbuf_write_histogram  (z_dbuf_writer_t *self,
                                  const z_histogram_t *histo);

__Z_END_DECLS__

#endif /* !_Z_DBUFFER_CODING_H_ */
