/*
 *   Copyright 2011 Matteo Bertozzi
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

#include <zcl/macros.h>
#include <zcl/types.h>

Z_TYPEDEF_CONST_STRUCT(z_stream_plug)
Z_TYPEDEF_STRUCT(z_stream_extent)
Z_TYPEDEF_STRUCT(z_stream)

#define Z_STREAM_EXTENT(x)                      Z_CAST(z_stream_extent_t, x)
#define Z_STREAM(x)                             Z_CAST(z_stream_t, x)

struct z_stream_plug {
    int          (*close)    (z_stream_t *stream);
    int          (*seek)     (z_stream_t *stream,
                              unsigned int offset);

    unsigned int (*read)     (z_stream_t *stream,
                              void *buffer,
                              unsigned int n);
    unsigned int (*write)    (z_stream_t *stream,
                              const void *buffer,
                              unsigned int n);
};

struct z_stream_extent {
    z_stream_t * stream;
    unsigned int offset;
    unsigned int length;
};

struct z_stream {
    z_stream_plug_t *plug;                      /* Stream Plugin */
    z_data_t         plug_data;                 /* Stream Plugin data */
    z_data_t         data;                      /* Extra Plugin data */
    unsigned int     offset;                    /* Stream Offset */
};


int     z_stream_open         (z_stream_t *stream,
                               z_stream_plug_t *plug);
int     z_stream_seek         (z_stream_t *stream,
                               unsigned int offset);
int     z_stream_close        (z_stream_t *stream);

int     z_stream_write        (z_stream_t *stream,
                               const void *buf,
                               unsigned int n);
int     z_stream_read         (z_stream_t *stream,
                               void *buf,
                               unsigned int n);
int     z_stream_pread        (z_stream_t *stream,
                               unsigned int offset,
                               void *buf,
                               unsigned int n);

int     z_stream_read_extent  (const z_stream_extent_t *extent,
                               void *buf);

unsigned int z_stream_read_line (z_stream_t *stream,
                                 char *line,
                                 unsigned int linesize);

int     z_stream_write_int8   (z_stream_t *stream, int8_t v);
int     z_stream_read_int8    (z_stream_t *stream, int8_t *v);

int     z_stream_write_uint8  (z_stream_t *stream, uint8_t v);
int     z_stream_read_uint8   (z_stream_t *stream, uint8_t *v);

int     z_stream_write_int16  (z_stream_t *stream, int16_t v);
int     z_stream_read_int16   (z_stream_t *stream, int16_t *v);

int     z_stream_write_uint16 (z_stream_t *stream, uint16_t v);
int     z_stream_read_uint16  (z_stream_t *stream, uint16_t *v);

int     z_stream_write_int32  (z_stream_t *stream, int32_t v);
int     z_stream_read_int32   (z_stream_t *stream, int32_t *v);

int     z_stream_write_uint32 (z_stream_t *stream, uint32_t v);
int     z_stream_read_uint32  (z_stream_t *stream, uint32_t *v);

int     z_stream_write_int64  (z_stream_t *stream, int64_t v);
int     z_stream_read_int64   (z_stream_t *stream, int64_t *v);

int     z_stream_write_uint64 (z_stream_t *stream, uint64_t v);
int     z_stream_read_uint64  (z_stream_t *stream, uint64_t *v);

int     z_stream_set_extent         (z_stream_t *stream,
                                     z_stream_extent_t *extent,
                                     unsigned int offset,
                                     unsigned int length);

unsigned int    z_stream_fetch      (z_stream_t *stream,
                                     unsigned int offset,
                                     unsigned int size,
                                     z_iopush_t fetch_func,
                                     void *user_data);
int             z_stream_memcmp     (z_stream_t *stream,
                                     unsigned int offset,
                                     const void *mem,
                                     unsigned int mem_size);

#endif /* !_Z_STREAM_H_ */

