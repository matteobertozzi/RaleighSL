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

#include <zcl/debug.h>
#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/writer.h>

void z_dump_int64  (FILE *stream, int64_t value) {
  fprintf(stream, "%"PRIi64, value);
}

void z_dump_uint64 (FILE *stream, uint64_t value) {
  fprintf(stream, "%"PRIu64, value);
}

void z_dump_buf (FILE *stream, const uint8_t *data, size_t size) {
  size_t i;
  fprintf(stream, "%zu:", size);
  for (i = 0; i < size; ++i) {
    fputc(data[i], stream);
  }
}

void z_dump_byte_slice (FILE *stream, const z_byte_slice_t *value) {
  z_dump_buf(stream, value->data, value->size);
}

void z_dump_buffer (FILE *stream, const z_buffer_t *value) {
  z_dump_buf(stream, value->block, value->size);
}

void z_dump_bytes (FILE *stream, const z_bytes_ref_t *value) {
  z_dump_byte_slice(stream, &(value->slice));
}

int z_write_field (z_buffer_t *buf, uint16_t field_id, size_t length) {
  if (Z_UNLIKELY(z_buffer_ensure(buf, 12 + length)))
    return(-1);

  int elen = z_encode_field(z_buffer_tail(buf), field_id, length);
  buf->size += elen;
  return(0);
}

int z_write_field_int64  (z_buffer_t *buf, uint16_t field_id, int64_t value) {
  return(z_write_field_uint64(buf, field_id, z_encode_zigzag64(value)));
}

int z_write_field_uint64 (z_buffer_t *buf, uint16_t field_id, uint64_t value) {
  unsigned int length = z_uint64_size(value);
  if (Z_UNLIKELY(z_write_field(buf, field_id, length)))
    return(-1);
  z_encode_uint64(z_buffer_tail(buf), length, value); buf->size += length;
  return(0);
}

int z_write_field_buf (z_buffer_t *buf, uint16_t field_id, const void *data, size_t size) {
  if (Z_UNLIKELY(z_write_field(buf, field_id, size)))
    return(-1);
  return(z_buffer_append(buf, data, size));
}

int z_write_field_byte_slice (z_buffer_t *buf, uint16_t field_id, const z_byte_slice_t *value) {
  return(z_write_field_buf(buf, field_id, value->data, value->size));
}

int z_write_field_buffer (z_buffer_t *buf, uint16_t field_id, const z_buffer_t *value) {
  return(z_write_field_buf(buf, field_id, value->block, value->size));
}

int z_write_field_bytes (z_buffer_t *buf, uint16_t field_id, const z_bytes_ref_t *value) {
  return(z_write_field_byte_slice(buf, field_id, &(value->slice)));
}

