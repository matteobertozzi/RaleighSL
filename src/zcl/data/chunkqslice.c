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

#include <zcl/chunkqslice.h>

static unsigned int __chunkq_slice_length (const z_slice_t *slice) {
    return(((const z_chunkq_slice_t *)slice)->extent->length);
}

static int __chunkq_slice_copy (const z_slice_t *slice,
                                unsigned int offset,
                                void *buffer,
                                unsigned int length)
{
    const z_chunkq_slice_t *ss = (const z_chunkq_slice_t *)slice;
    return(z_chunkq_read(ss->extent->chunkq,
                         ss->extent->offset + offset, buffer, length));
}

static int __chunkq_slice_rawcmp (const z_slice_t *slice,
                                  unsigned int offset,
                                  const void *blob,
                                  unsigned int length)
{
    const z_chunkq_slice_t *cs = (const z_chunkq_slice_t *)slice;
    return(z_chunkq_memcmp(cs->extent->chunkq, offset, blob, length));
}

static int __chunkq_slice_fetch (const z_slice_t *slice,
                                 unsigned int offset,
                                 unsigned int size,
                                 z_iopush_t func,
                                 void *user_data)
{
    const z_chunkq_slice_t *cs = (const z_chunkq_slice_t *)slice;
    z_chunkq_fetch(cs->extent->chunkq, cs->extent->offset + offset,
                   size, func, user_data);
    return(0);
}

static uint8_t __chunkq_slice_fetch8 (const z_slice_t *slice, unsigned int offset) {
    const z_chunkq_slice_t *ss = (const z_chunkq_slice_t *)slice;
    uint8_t u8 = 0;
    z_chunkq_read(ss->extent->chunkq, ss->extent->offset + offset, &u8, 1);
    return(u8);
}

static uint16_t __chunkq_slice_fetch16 (const z_slice_t *slice, unsigned int offset) {
    const z_chunkq_slice_t *ss = (const z_chunkq_slice_t *)slice;
    uint16_t u16 = 0;
    z_chunkq_read(ss->extent->chunkq, ss->extent->offset + offset, &u16, 2);
    return(u16);
}

static uint32_t __chunkq_slice_fetch32 (const z_slice_t *slice, unsigned int offset) {
    const z_chunkq_slice_t *ss = (const z_chunkq_slice_t *)slice;
    uint32_t u32 = 0;
    z_chunkq_read(ss->extent->chunkq, ss->extent->offset + offset, &u32, 4);
    return(u32);
}

static uint64_t __chunkq_slice_fetch64 (const z_slice_t *slice, unsigned int offset) {
    const z_chunkq_slice_t *ss = (const z_chunkq_slice_t *)slice;
    uint64_t u64;
    z_chunkq_read(ss->extent->chunkq, ss->extent->offset + offset, &u64, 8);
    return(u64);
}

static z_slice_vtable_t __chunkq_slice = {
    .length  = __chunkq_slice_length,
    .copy    = __chunkq_slice_copy,
    .write   = NULL,
    .rawcmp  = __chunkq_slice_rawcmp,
    .fetch   = __chunkq_slice_fetch,
    .fetch8  = __chunkq_slice_fetch8,
    .fetch16 = __chunkq_slice_fetch16,
    .fetch32 = __chunkq_slice_fetch32,
    .fetch64 = __chunkq_slice_fetch64,
};

void z_chunkq_slice (z_chunkq_slice_t *slice,
                     const z_chunkq_extent_t *chunkq_ext)
{
    slice->__base_type__.vtable = &__chunkq_slice;
    slice->extent = chunkq_ext;
}

