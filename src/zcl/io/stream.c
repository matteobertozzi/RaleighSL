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

#include <zcl/stream.h>
#include <zcl/memcpy.h>
#include <zcl/memchr.h>
#include <zcl/memcmp.h>

static unsigned int __stream_fetch (z_stream_t *stream,
                                    unsigned int offset,
                                    unsigned int length,
                                    z_iopush_t fetch_func,
                                    void *user_data)
{
    unsigned int n, rd;
    char buffer[1024];

    n = length;
    while (n > 0) {
        rd = (n > sizeof(buffer)) ? sizeof(buffer) : n;

        rd = z_stream_pread(stream, offset, buffer, rd);
        fetch_func(user_data, buffer, rd);

        n -= rd;
        offset += rd;
    }

    return(length);
}

static int __stream_memcmp (z_stream_t *stream,
                           unsigned int offset,
                           const void *mem,
                           unsigned int mem_size)
{
    const char *p = (const char *)mem;
    unsigned int n, rd;
    char buffer[1024];
    int cmp;

    n = mem_size;
    while (n > 0) {
        rd = (n > sizeof(buffer)) ? sizeof(buffer) : n;

        rd = z_stream_pread(stream, offset, buffer, rd);
        if ((cmp = z_memcmp(buffer, p, rd)) != 0)
            return(cmp);

        n -= rd;
        p += rd;
        offset += rd;
    }

    return(0);
}

int z_stream_open (z_stream_t *stream, z_stream_plug_t *plug) {
    stream->plug = plug;
    stream->offset = 0;
    return(0);
}

int z_stream_seek (z_stream_t *stream, unsigned int offset) {
    z_stream_plug_t *plug = stream->plug;

    if (plug->seek != NULL && plug->seek(stream, offset))
        return(-1);

    stream->offset = offset;
    return(0);
}

int z_stream_close (z_stream_t *stream) {
    z_stream_plug_t *plug = stream->plug;

    if (plug->close != NULL)
        return(plug->close(stream));

    return(0);
}


int z_stream_write (z_stream_t *stream, const void *buf, unsigned int n) {
    return(stream->plug->write(stream, buf, n));
}

int z_stream_read (z_stream_t *stream, void *buf, unsigned int n) {
    return(stream->plug->read(stream, buf, n));
}

int z_stream_pread (z_stream_t *stream,
                    unsigned int offset,
                    void *buf,
                    unsigned int length)
{
    unsigned int old_offset;
    int rd;

    old_offset = stream->offset;
    z_stream_seek(stream, offset);
    rd = z_stream_read(stream, buf, length);
    z_stream_seek(stream, old_offset);

    return(rd);
}

int z_stream_read_extent (const z_stream_extent_t *extent, void *buf) {
    return(z_stream_pread(extent->stream, extent->offset, buf, extent->length));
}

int z_stream_write_int8 (z_stream_t *stream, int8_t v) {
    return(stream->plug->write(stream, &v, 1));
}

int z_stream_read_int8 (z_stream_t *stream, int8_t *v) {
    if (stream->plug->read(stream, v, 1) != 1)
        return(-1);
    return(0);
}

int z_stream_write_uint8 (z_stream_t *stream, uint8_t v) {
    return(stream->plug->write(stream, &v, 1));
}

int z_stream_read_uint8 (z_stream_t *stream, uint8_t *v) {
    if (stream->plug->read(stream, v, 1) != 1)
        return(-1);
    return(0);
}


int z_stream_write_int16 (z_stream_t *stream, int16_t v) {
    uint8_t buffer[2];

    buffer[0] = (v >> 8);
    buffer[1] = (v >> 0);

    return(stream->plug->write(stream, buffer, 2));
}

int z_stream_read_int16 (z_stream_t *stream, int16_t *v) {
    uint8_t buffer[2];
    int16_t v16;

    if (stream->plug->read(stream, buffer, 2) != 2)
        return(-1);

    v16  = ((uint16_t)(buffer[0]) << 8);
    v16 += ((uint16_t)(buffer[1]) << 0);
    *v = v16;

    return(0);
}

int z_stream_write_uint16 (z_stream_t *stream, uint16_t v) {
    uint8_t buffer[2];

    buffer[0] = (v >> 8);
    buffer[1] = (v >> 0);

    return(stream->plug->write(stream, buffer, 2));
}

int z_stream_read_uint16 (z_stream_t *stream, uint16_t *v) {
    uint8_t buffer[2];
    uint16_t v16;

    if (stream->plug->read(stream, buffer, 2) != 2)
        return(-1);

    v16  = ((uint16_t)(buffer[0]) << 8);
    v16 += ((uint16_t)(buffer[1]) << 0);
    *v = v16;

    return(0);
}


int z_stream_write_int32 (z_stream_t *stream, int32_t v) {
    uint8_t buffer[4];

    buffer[0] = (v >> 24);
    buffer[1] = (v >> 16);
    buffer[2] = (v >>  8);
    buffer[3] = (v >>  0);

    return(stream->plug->write(stream, buffer, 4));
}

int z_stream_read_int32 (z_stream_t *stream, int32_t *v) {
    uint8_t buffer[4];
    int32_t v32;

    if (stream->plug->read(stream, buffer, 4) != 4)
        return(-1);

    v32  = ((uint32_t)(buffer[0]) << 24);
    v32 += ((uint32_t)(buffer[1]) << 16);
    v32 += ((uint32_t)(buffer[2]) <<  8);
    v32 += ((uint32_t)(buffer[3]) <<  0);
    *v = v32;

    return(0);
}

int z_stream_write_uint32 (z_stream_t *stream, uint32_t v) {
    uint8_t buffer[4];

    buffer[0] = (v >> 24);
    buffer[1] = (v >> 16);
    buffer[2] = (v >>  8);
    buffer[3] = (v >>  0);

    return(stream->plug->write(stream, buffer, 4));
}

int z_stream_read_uint32 (z_stream_t *stream, uint32_t *v) {
    uint8_t buffer[4];
    uint32_t v32;

    if (stream->plug->read(stream, buffer, 4) != 4)
        return(-1);

    v32  = ((uint32_t)(buffer[0]) << 24);
    v32 += ((uint32_t)(buffer[1]) << 16);
    v32 += ((uint32_t)(buffer[2]) <<  8);
    v32 += ((uint32_t)(buffer[3]) <<  0);
    *v = v32;

    return(0);
}


int z_stream_write_int64 (z_stream_t *stream, int64_t v) {
    int8_t buffer[8];

    buffer[0] = (v >> 56);
    buffer[1] = (v >> 48);
    buffer[2] = (v >> 40);
    buffer[3] = (v >> 32);
    buffer[4] = (v >> 24);
    buffer[5] = (v >> 16);
    buffer[6] = (v >>  8);
    buffer[7] = (v >>  0);

    return(stream->plug->write(stream, buffer, 8));
}

int z_stream_read_int64 (z_stream_t *stream, int64_t *v) {
    uint8_t buffer[8];
    int64_t v64;

    if (stream->plug->read(stream, buffer, 8) != 8)
        return(-1);

    v64  = ((uint64_t)(buffer[0]) << 56);
    v64 += ((uint64_t)(buffer[1]) << 48);
    v64 += ((uint64_t)(buffer[2]) << 40);
    v64 += ((uint64_t)(buffer[3]) << 32);
    v64 += ((uint64_t)(buffer[4]) << 24);
    v64 += ((uint64_t)(buffer[5]) << 16);
    v64 += ((uint64_t)(buffer[6]) <<  8);
    v64 += ((uint64_t)(buffer[7]) <<  0);
    *v = v64;

    return(0);
}

int z_stream_write_uint64 (z_stream_t *stream, uint64_t v) {
    uint8_t buffer[8];

    buffer[0] = (v >> 56);
    buffer[1] = (v >> 48);
    buffer[2] = (v >> 40);
    buffer[3] = (v >> 32);
    buffer[4] = (v >> 24);
    buffer[5] = (v >> 16);
    buffer[6] = (v >>  8);
    buffer[7] = (v >>  0);

    return(stream->plug->write(stream, buffer, 8));
}

int z_stream_read_uint64 (z_stream_t *stream, uint64_t *v) {
    uint8_t buffer[8];
    uint64_t v64;

    if (stream->plug->read(stream, buffer, 8) != 8)
        return(-1);

    v64  = ((uint64_t)(buffer[0]) << 56);
    v64 += ((uint64_t)(buffer[1]) << 48);
    v64 += ((uint64_t)(buffer[2]) << 40);
    v64 += ((uint64_t)(buffer[3]) << 32);
    v64 += ((uint64_t)(buffer[4]) << 24);
    v64 += ((uint64_t)(buffer[5]) << 16);
    v64 += ((uint64_t)(buffer[6]) <<  8);
    v64 += ((uint64_t)(buffer[7]) <<  0);
    *v = v64;

    return(0);
}

unsigned int z_stream_read_line (z_stream_t *stream,
                                 char *line,
                                 unsigned int linesize)
{
    unsigned int consumed;
    unsigned int rd;
    char buffer[64];
    char *p;

    consumed = 0;
    while ((rd = stream->plug->read(stream, buffer, sizeof(buffer))) > 0) {
        if ((p = z_memchr(buffer, '\n', rd)) != NULL) {
            unsigned int n = p - buffer;

            stream->offset -= (rd - n);
            z_memcpy(line + consumed, buffer, n);
            return(consumed + n);
        }

        if ((consumed + rd) >= linesize)
            return(linesize + 1);

        consumed += rd;
        z_memcpy(line + consumed, buffer, rd);
    }

    return(0);
}

int z_stream_set_extent (z_stream_t *stream,
                         z_stream_extent_t *extent,
                         unsigned int offset,
                         unsigned int length)
{
    extent->stream = stream;
    extent->offset = offset;
    extent->length = length;
    return(0);
}

unsigned int z_stream_fetch (z_stream_t *stream,
                             unsigned int offset,
                             unsigned int length,
                             z_iopush_t fetch_func,
                             void *user_data)
{
    unsigned int old_offset;
    int res;

    if (stream->plug->fetch == NULL)
        return(__stream_fetch(stream, offset, length, fetch_func, user_data));

    old_offset = stream->offset;
    z_stream_seek(stream, offset);
    res = stream->plug->fetch(stream, length, fetch_func, user_data);
    z_stream_seek(stream, old_offset);

    return(res);
}

int z_stream_memcmp (z_stream_t *stream,
                     unsigned int offset,
                     const void *mem,
                     unsigned int mem_size)
{
    unsigned int old_offset;
    int cmp;

    if (stream->plug->memcmp == NULL)
        return(__stream_memcmp(stream, offset, mem, mem_size));

    old_offset = stream->offset;
    z_stream_seek(stream, offset);
    cmp = stream->plug->memcmp(stream, mem, mem_size);
    z_stream_seek(stream, old_offset);

    return(cmp);
}

