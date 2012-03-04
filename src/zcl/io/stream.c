/*
 *   Copyright 2012 Matteo Bertozzi
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stream.h"

/* ============================================================================
 *  Writable
 */
int z_stream_write_fully (z_stream_t *stream, 
                          const void *buffer, 
                          unsigned int size) 
{
    const uint8_t *pbuf = (const uint8_t *)buffer;
    int n = 0;
    int wr;

    while (size > 0) {
        if ((wr = z_stream_write(stream, pbuf, size)) <= 0)
            break;

        n += wr;
        pbuf += wr;
        size -= wr;
    }

    return(n);
}

int z_stream_pwrite (z_stream_t *stream,
                     uint64_t offset,
                     const void *buffer,
                     unsigned int size)
{
    uint64_t pos;
    int wr;
    
    pos = z_stream_tell(stream);
    z_stream_seek(stream, offset);
    wr = z_stream_write_fully(stream, buffer, size);
    z_stream_seek(stream, pos);
    
    return(wr);
}

int z_stream_write_uint8 (z_stream_t *stream, uint8_t value) {
    return(z_stream_write_fully(stream, &value, 1));
}

int z_stream_write_uint16 (z_stream_t *stream, uint16_t value) {
    uint8_t buffer[2];
    buffer[0] = (value >> 8) & 0xff;
    buffer[1] = (value >> 0) & 0xff;
    return(z_stream_write_fully(stream, buffer, 2));
}

int z_stream_write_uint32 (z_stream_t *stream, uint32_t value) {
    uint8_t buffer[4];
    buffer[0] = (value >> 24) & 0xff;
    buffer[1] = (value >> 16) & 0xff;
    buffer[2] = (value >> 8) & 0xff;
    buffer[3] = (value >> 0) & 0xff;
    return(z_stream_write_fully(stream, buffer, 4));
}

int z_stream_write_uint64 (z_stream_t *stream, uint64_t value) {
    uint8_t buffer[8];
    buffer[0] = (value >> 56) & 0xff;
    buffer[1] = (value >> 48) & 0xff;
    buffer[2] = (value >> 40) & 0xff;
    buffer[3] = (value >> 32) & 0xff;
    buffer[4] = (value >> 24) & 0xff;
    buffer[5] = (value >> 16) & 0xff;
    buffer[6] = (value >> 8) & 0xff;
    buffer[7] = (value >> 0) & 0xff;
    return(z_stream_write_fully(stream, buffer, 8));
}

int z_stream_write_vuint (z_stream_t *stream, uint64_t value) {
    unsigned int length = 0;
    uint8_t buffer[10];

    while (value >= 128) {
        buffer[length++] = (value & 0x7f) | 128;
        value >>= 7;
    }
    buffer[length++] = value & 0xff;

    return(z_stream_write_fully(stream, buffer, length));
}

int z_stream_write_int8 (z_stream_t *stream, int8_t value) {
    return(z_stream_write_uint8(stream, (uint8_t)value));
}

int z_stream_write_int16 (z_stream_t *stream, int16_t value) {
    return(z_stream_write_uint16(stream, (uint16_t)value));
}

int z_stream_write_int32 (z_stream_t *stream, int32_t value) {
    return(z_stream_write_uint32(stream, (uint32_t)value));
}

int z_stream_write_int64 (z_stream_t *stream, int64_t value) {
    return(z_stream_write_uint64(stream, (uint64_t)value));
}

int z_stream_write_vint (z_stream_t *stream, int64_t value) {
    uint64_t v = (value << 1) ^ (value >> 63);
    return(z_stream_write_vuint(stream, v));
}

/* ============================================================================
 *  Readable
 */
int z_stream_read_fully (z_stream_t *stream, void *buffer, unsigned int size) {
    uint8_t *pbuf = (uint8_t *)buffer;
    int n = 0;
    int rd;

    while (size > 0) {
        if ((rd = z_stream_read(stream, pbuf, size)) <= 0)
            break;

        n += rd;
        pbuf += rd;
        size -= rd;
    }

    return(n);
}

int z_stream_pread (z_stream_t *stream,
                    uint64_t offset,
                    void *buffer,
                    unsigned int size)
{
    uint64_t pos;
    int rd;
    
    pos = z_stream_tell(stream);
    z_stream_seek(stream, offset);
    rd = z_stream_read_fully(stream, buffer, size);
    z_stream_seek(stream, pos);
    
    return(rd);
}

int z_stream_read_uint8 (z_stream_t *stream, uint8_t *value) {
    return(z_stream_read(stream, value, 1));
}

int z_stream_read_uint16 (z_stream_t *stream, uint16_t *value) {
    uint8_t buffer[2];
    int rd;

    if ((rd = z_stream_read_fully(stream, buffer, 2)) != 2)
        return(rd);

    *value = ((uint16_t)(buffer[0]) <<  8) +
             ((uint16_t)(buffer[1]) <<  0);

    return(rd);
}

int z_stream_read_uint32 (z_stream_t *stream, uint32_t *value) {
    uint8_t buffer[4];
    int rd;

    if ((rd = z_stream_read_fully(stream, buffer, 4)) != 4)
        return(rd);

    *value = ((uint32_t)(buffer[0]) << 24) +
             ((uint32_t)(buffer[1]) << 16) +
             ((uint32_t)(buffer[2]) <<  8) +
             ((uint32_t)(buffer[3]) <<  0);

    return(rd);
}

int z_stream_read_uint64 (z_stream_t *stream, uint64_t *value) {
    uint8_t buffer[8];
    int rd;

    if ((rd = z_stream_read_fully(stream, buffer, 8)) != 8)
        return(rd);

    *value = ((uint64_t)(buffer[0]) << 56) +
             ((uint64_t)(buffer[1]) << 48) +
             ((uint64_t)(buffer[2]) << 40) +
             ((uint64_t)(buffer[3]) << 32) +
             ((uint64_t)(buffer[4]) << 24) +
             ((uint64_t)(buffer[5]) << 16) +
             ((uint64_t)(buffer[6]) <<  8) +
             ((uint64_t)(buffer[7]) <<  0);

    return(rd);
}

int z_stream_read_vuint (z_stream_t *stream, uint64_t *value) {
    uint64_t result = 0;
    unsigned int shift;
    uint8_t buffer;
    int rd = 0;

    for (shift = 0; shift < 64; shift += 7) {
        if (z_stream_read(stream, &buffer, 1) != 1)
            return(-rd);

        rd++;
        if (buffer & 128) {
            result |= ((uint64_t)(buffer & 0x7f) << shift);
        } else {
            result |= ((uint64_t)buffer << shift);
            break;
        }
    }

    *value = result;
    return(rd);
}

int z_stream_read_int8 (z_stream_t *stream, int8_t *value) {
    return(z_stream_read_uint8(stream, (uint8_t *)value));
}

int z_stream_read_int16 (z_stream_t *stream, int16_t *value) {
    return(z_stream_read_uint16(stream, (uint16_t *)value));
}

int z_stream_read_int32 (z_stream_t *stream, int32_t *value) {
    return(z_stream_read_uint32(stream, (uint32_t *)value));
}

int z_stream_read_int64 (z_stream_t *stream, int64_t *value) {
    return(z_stream_read_uint64(stream, (uint64_t *)value));
}

int z_stream_read_vint (z_stream_t *stream, int64_t *value) {
    uint64_t v;
    int rd;

    rd = z_stream_read_vuint(stream, &v);
    *value = (int64_t)((v >> 1) ^ -(v & 1));

    return(rd);
}
