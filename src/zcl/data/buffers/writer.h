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

#ifndef _Z_WRITER_H_
#define _Z_WRITER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memslice.h>
#include <zcl/dbuffer.h>
#include <zcl/macros.h>
#include <zcl/memref.h>

#include <stdio.h>

#define z_dump_int8                 z_dump_int16
#define z_dump_int16                z_dump_int32
#define z_dump_int32                z_dump_int64

#define z_dump_uint8                z_dump_uint16
#define z_dump_uint16               z_dump_uint32
#define z_dump_uint32               z_dump_uint64

#define z_write_field_int8          z_write_field_int16
#define z_write_field_int16         z_write_field_int32
#define z_write_field_int32         z_write_field_int64

#define z_write_field_uint8         z_write_field_uint16
#define z_write_field_uint16        z_write_field_uint32
#define z_write_field_uint32        z_write_field_uint64

void z_dump_int64   (FILE *stream, int64_t value);
void z_dump_uint64  (FILE *stream, uint64_t value);
void z_dump_slice   (FILE *stream, const z_memslice_t *value);
void z_dump_bytes   (FILE *stream, const z_memref_t *value);
void z_dump_string  (FILE *stream, const z_memref_t *value);

int z_write_field        (z_dbuf_writer_t *buf, uint16_t field_id, size_t length);
int z_write_field_int64  (z_dbuf_writer_t *buf, uint16_t field_id, int64_t value);
int z_write_field_uint64 (z_dbuf_writer_t *buf, uint16_t field_id, uint64_t value);
int z_write_field_slice  (z_dbuf_writer_t *buf, uint16_t field_id, const z_memslice_t *value);
int z_write_field_bytes  (z_dbuf_writer_t *buf, uint16_t field_id, const z_memref_t *value);
int z_write_field_string (z_dbuf_writer_t *buf, uint16_t field_id, const z_memref_t *value);

#endif /* !_Z_WRITER_H_ */
