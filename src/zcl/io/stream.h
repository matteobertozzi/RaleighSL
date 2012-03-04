/*
 *   Copyright 2011-2012 Matteo Bertozzi
 *
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

#ifndef _Z_STREAM_H_
#define _Z_STREAM_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/types.h>

#define Z_STREAM(x)                             Z_CAST(z_stream_t, x)

Z_TYPEDEF_CONST_STRUCT(z_stream_vtable)
Z_TYPEDEF_STRUCT(z_stream)

struct z_stream_vtable {
    int      (*write)     (z_stream_t *stream, const void *buf, unsigned int n);
    int      (*flush)     (z_stream_t *stream);
    int      (*zread)     (z_stream_t *stream, void **buf, unsigned int n);
    int      (*read)      (z_stream_t *stream, void *buf, unsigned int n);
    int      (*seek)      (z_stream_t *stream, uint64_t offset);
    uint64_t (*tell)      (z_stream_t *stream);
    
    int      (*can_write) (z_stream_t *stream);
    int      (*can_zread) (z_stream_t *stream);
    int      (*can_read)  (z_stream_t *stream);
    int      (*can_seek)  (z_stream_t *stream);

    uint64_t (*position)  (z_stream_t *stream);
    uint64_t (*length)    (z_stream_t *stream);
};

struct z_stream {
    z_stream_vtable_t *vtable;
};

struct z_stream_plug {
    z_stream_t __base_type__;
    z_data_t plug_data;
    z_data_t data;
    unsigned int offset;
};

#define z_stream_call(stream, method, ...)                                  \
    Z_VTABLE_CALL(z_stream_t, stream, method, ##__VA_ARGS__)

#define z_stream_can(stream, method)                                       \
    (Z_VTABLE_HAS_METHOD(z_stream_t, stream, can_ ## method) ?             \
        Z_VTABLE_CALL(z_stream_t, stream, can_ ## method) :                \
        Z_VTABLE_HAS_METHOD(z_stream_t, stream, method))

#define z_stream_write(stream, buf, n)      z_stream_call(stream, write, buf, n)
#define z_stream_read(stream, buf, n)       z_stream_call(stream, read, buf, n)
#define z_stream_flush(stream)              z_stream_call(stream, flush)
#define z_stream_tell(stream)               z_stream_call(stream, tell)
#define z_stream_seek(stream, offset)       z_stream_call(stream, seek, offset)
#define z_stream_position(stream)           z_stream_call(stream, position)
#define z_stream_length(stream)             z_stream_call(stream, length)

#define z_stream_can_write(stream)          z_stream_can(stream, write)
#define z_stream_can_read(stream)           z_stream_can(stream, read)

int     z_stream_write_fully  (z_stream_t *stream,
                               const void *buffer,
                               unsigned int size);
int     z_stream_read_fully   (z_stream_t *stream,
                               void *buffer,
                               unsigned int size);

int     z_stream_pwrite       (z_stream_t *stream,
                               uint64_t offset,
                               const void *buffer,
                               unsigned int size);
int     z_stream_pread        (z_stream_t *stream,
                               uint64_t offset,
                               void *buffer,
                               unsigned int size);

int     z_stream_write_uint8  (z_stream_t *stream, uint8_t value);
int     z_stream_write_uint16 (z_stream_t *stream, uint16_t value);
int     z_stream_write_uint32 (z_stream_t *stream, uint32_t value);
int     z_stream_write_uint64 (z_stream_t *stream, uint64_t value);
int     z_stream_write_vuint  (z_stream_t *stream, uint64_t value);

int     z_stream_write_int8   (z_stream_t *stream, int8_t value);
int     z_stream_write_int16  (z_stream_t *stream, int16_t value);
int     z_stream_write_int32  (z_stream_t *stream, int32_t value);
int     z_stream_write_int64  (z_stream_t *stream, int64_t value);
int     z_stream_write_vint   (z_stream_t *stream, int64_t value);

int     z_stream_read_uint8   (z_stream_t *stream, uint8_t *value);
int     z_stream_read_uint16  (z_stream_t *stream, uint16_t *value);
int     z_stream_read_uint32  (z_stream_t *stream, uint32_t *value);
int     z_stream_read_uint64  (z_stream_t *stream, uint64_t *value);
int     z_stream_read_vuint   (z_stream_t *stream, uint64_t *value);

int     z_stream_read_int8    (z_stream_t *stream, int8_t *value);
int     z_stream_read_int16   (z_stream_t *stream, int16_t *value);
int     z_stream_read_int32   (z_stream_t *stream, int32_t *value);
int     z_stream_read_int64   (z_stream_t *stream, int64_t *value);
int     z_stream_read_vint    (z_stream_t *stream, int64_t *value);

__Z_END_DECLS__

#endif /* !_Z_STREAM_H_ */

