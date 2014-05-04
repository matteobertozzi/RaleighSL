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
#include <zcl/writer.h>
#include <zcl/debug.h>

void z_dump_int64  (FILE *stream, int64_t value) {
  fprintf(stream, "%"PRIi64, value);
}

void z_dump_uint64 (FILE *stream, uint64_t value) {
  fprintf(stream, "%"PRIu64, value);
}

void z_dump_data (FILE *stream, const uint8_t *data, size_t size) {
  size_t i;
  fprintf(stream, "%zu:", size);
  for (i = 0; i < size; ++i) {
    fputc(data[i], stream);
  }
}

void z_dump_slice (FILE *stream, const z_memslice_t *value) {
  z_dump_data(stream, value->data, value->size);
}

void z_dump_bytes (FILE *stream, const z_memref_t *value) {
  z_dump_slice(stream, &(value->slice));
}

void z_dump_string (FILE *stream, const z_memref_t *value) {
  z_dump_slice(stream, &(value->slice));
}

int z_write_field (z_dbuf_writer_t *writer, uint16_t field_id, size_t length) {
  uint8_t head[16];
  int elen;
  elen = z_field_encode(head, field_id, length);
  return(z_dbuf_writer_add(writer, head, elen));
}

int z_write_field_int64  (z_dbuf_writer_t *writer, uint16_t field_id, int64_t value) {
  return(z_write_field_uint64(writer, field_id, z_zigzag64_encode(value)));
}

int z_write_field_uint64 (z_dbuf_writer_t *writer, uint16_t field_id, uint64_t value) {
  uint8_t block[24];
  unsigned int length;
  int elen;

  length = z_uint64_size(value);
  elen = z_field_encode(block, field_id, length);
  z_uint64_encode(block + elen, length, value);
  return(z_dbuf_writer_add(writer, block, elen + length));
}

int z_write_field_data (z_dbuf_writer_t *writer, uint16_t field_id, const void *data, size_t size) {
  if (Z_UNLIKELY(z_write_field(writer, field_id, size)))
    return(-1);
  return(z_dbuf_writer_add(writer, data, size));
}

int z_write_field_memslice (z_dbuf_writer_t *writer, uint16_t field_id, const z_memslice_t *value) {
  return(z_write_field_data(writer, field_id, value->data, value->size));
}

int z_write_field_bytes (z_dbuf_writer_t *writer, uint16_t field_id, const z_memref_t *value) {
#if 1
  if (Z_UNLIKELY(z_write_field(writer, field_id, value->slice.size)))
    return(-1);
  return(z_dbuf_writer_add_ref(writer, value));
#else
  return(z_write_field_memslice(writer, field_id, &(value->slice)));
#endif
}

int z_write_field_string (z_dbuf_writer_t *writer, uint16_t field_id, const z_memref_t *value) {
  if (Z_UNLIKELY(z_write_field(writer, field_id, value->slice.size)))
    return(-1);
  return(z_dbuf_writer_add_ref(writer, value));
}
