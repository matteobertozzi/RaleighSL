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

#include <zcl/memcpy.h>
#include <zcl/memcmp.h>

#include <zcl/streamslice.h>

static unsigned int __stream_slice_length (const z_slice_t *slice) {
    return(((const z_stream_slice_t *)slice)->length);
}

static int __stream_slice_copy (const z_slice_t *slice,
                                unsigned int offset,
                                void *buffer,
                                unsigned int length)
{
    const z_stream_slice_t *ss = (const z_stream_slice_t *)slice;
    return(z_stream_pread(ss->stream, ss->offset + offset, buffer, length));
}

#include <stdio.h>
static int __stream_slice_write (const z_slice_t *slice,
                                 z_stream_t *stream)
{
    fprintf(stderr, "__stream_slice_write(): TODO\n");
    /* TODO */
    return(0);
}

static int __stream_slice_rawcmp (const z_slice_t *slice,
                                  unsigned int offset,
                                  const void *blob,
                                  unsigned int length)
{
    const z_stream_slice_t *ss = (const z_stream_slice_t *)slice;
    const unsigned char *p = (const unsigned char *)blob;
    unsigned int n, rd;
    char buffer[512];
    int cmp;
    
    /* TODO: fixme to act as memncmp() */
    n = length;
    offset = ss->offset + offset;
    while (n > 0) {
        rd = (n > sizeof(buffer)) ? sizeof(buffer) : n;
        if ((rd = z_stream_pread(ss->stream, offset, buffer, rd)) <= 0)
            return(-1);
        
        if ((cmp = z_memcmp(buffer, p, rd)) != 0)
            return(cmp);
        
        offset += rd;
        p += rd;
        n -= rd;
    }
    
    return(0);
}

static int __stream_slice_fetch (const z_slice_t *slice,
                                 unsigned int offset,
                                 unsigned int size,
                                 z_iopush_t func,
                                 void *user_data)
{
    const z_stream_slice_t *ss = (const z_stream_slice_t *)slice;
    char buffer[512];
    int n, rd;

    n = size;
    offset = ss->offset + offset;
    while (size > 0) {
        rd = (n > sizeof(buffer)) ? sizeof(buffer) : n;
        if ((rd = z_stream_pread(ss->stream, offset, buffer, rd)) <= 0)
            break;
        
        func(user_data, buffer, rd);
        offset += rd;
        n -= rd;
    }

    return(size - n);
}

static uint8_t __stream_slice_fetch8 (const z_slice_t *slice, unsigned int offset) {
    const z_stream_slice_t *ss = (const z_stream_slice_t *)slice;
    uint8_t u8 = 0;
    z_stream_pread(ss->stream, ss->offset + offset, &u8, 1);
    return(u8);
}

static uint16_t __stream_slice_fetch16 (const z_slice_t *slice, unsigned int offset) {
    const z_stream_slice_t *ss = (const z_stream_slice_t *)slice;
    uint16_t u16 = 0;
    z_stream_pread(ss->stream, ss->offset + offset, &u16, 2);
    return(u16);
}

static uint32_t __stream_slice_fetch32 (const z_slice_t *slice, unsigned int offset) {
    const z_stream_slice_t *ss = (const z_stream_slice_t *)slice;
    uint32_t u32 = 0;
    z_stream_pread(ss->stream, ss->offset + offset, &u32, 4);
    return(u32);
}

static uint64_t __stream_slice_fetch64 (const z_slice_t *slice, unsigned int offset) {
    const z_stream_slice_t *ss = (const z_stream_slice_t *)slice;
    uint64_t u64;
    z_stream_pread(ss->stream, ss->offset + offset, &u64, 8);
    return(u64);
}

static z_slice_vtable_t __stream_slice = {
    .length  = __stream_slice_length,
    .copy    = __stream_slice_copy,
    .write   = __stream_slice_write,
    .rawcmp  = __stream_slice_rawcmp,
    .fetch   = __stream_slice_fetch,
    .fetch8  = __stream_slice_fetch8,
    .fetch16 = __stream_slice_fetch16,
    .fetch32 = __stream_slice_fetch32,
    .fetch64 = __stream_slice_fetch64,
};

void z_stream_slice (z_stream_slice_t *slice,
                     z_stream_t *stream,
                     unsigned int offset,
                     unsigned int length)
{
    slice->__base_type__.vtable = &__stream_slice;
    slice->stream = stream;
    slice->offset = offset;
    slice->length = length;
}
