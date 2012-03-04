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

#include <zcl/byteslice.h>

/* ============================================================================
 *  Byte Slice
 */
static unsigned int __byte_slice_length (const z_slice_t *slice) {
    return(((const z_byte_slice_t *)slice)->size);
}

static int __byte_slice_copy (const z_slice_t *slice,
                              unsigned int offset,
                              void *buffer,
                              unsigned int length)
{
    const z_byte_slice_t *bs = (const z_byte_slice_t *)slice;
    unsigned int n;

    if (offset > bs->size)
        return(0);

    if ((n = (bs->size - offset)) > length)
        n = length;

    z_memcpy(buffer, bs->data + offset, n);
    return(n);
}

static int __byte_slice_write (const z_slice_t *slice,
                               z_stream_t *stream)
{
    const z_byte_slice_t *bs = (const z_byte_slice_t *)slice;
    return(z_stream_write_fully(stream, bs->data, bs->size));
}


static int __byte_slice_rawcmp (const z_slice_t *slice,
                                unsigned int offset,
                                const void *blob,
                                unsigned int length)
{
    const z_byte_slice_t *bs = (const z_byte_slice_t *)slice;
    return(z_memncmp(bs->data + offset, bs->size - offset, blob, length));
}

int __byte_slice_fetch (const z_slice_t *slice,
                        unsigned int offset,
                        unsigned int size,
                        z_iopush_t func,
                        void *user_data)
{
    const z_byte_slice_t *bs = (const z_byte_slice_t *)slice;
    func(user_data, bs->data + offset, size);
    return(0);
}

uint8_t __byte_slice_fetch8 (const z_slice_t *slice, unsigned int offset) {
    return(*(Z_CONST_BYTE_SLICE(slice)->data + offset));
}

uint16_t __byte_slice_fetch16 (const z_slice_t *slice, unsigned int offset) {
    const uint16_t *data = Z_CONST_UINT16_PTR(Z_CONST_BYTE_SLICE(slice)->data);
    return(*(data + offset));
}

uint32_t __byte_slice_fetch32 (const z_slice_t *slice, unsigned int offset) {
    const uint32_t *data = Z_CONST_UINT32_PTR(Z_CONST_BYTE_SLICE(slice)->data);
    return(*(data + offset));
}

uint64_t __byte_slice_fetch64 (const z_slice_t *slice, unsigned int offset) {
    const uint64_t *data = Z_CONST_UINT64_PTR(Z_CONST_BYTE_SLICE(slice)->data);
    return(*(data + offset));
}

static z_slice_vtable_t __byte_slice = {
    .length  = __byte_slice_length,
    .copy    = __byte_slice_copy,
    .write   = __byte_slice_write,
    .rawcmp  = __byte_slice_rawcmp,
    .fetch   = __byte_slice_fetch,
    .fetch8  = __byte_slice_fetch8,
    .fetch16 = __byte_slice_fetch16,
    .fetch32 = __byte_slice_fetch32,
    .fetch64 = __byte_slice_fetch64,
};

void z_byte_slice (z_byte_slice_t *slice,
                   const void *data,
                   unsigned int size)
{
    slice->__base_type__.vtable = &__byte_slice;
    slice->data = data;
    slice->size = size;
}


/* ============================================================================
 *  Prefix (Byte) Slice
 */
static unsigned int __prefix_slice_length (const z_slice_t *slice) {
    const z_prefix_slice_t *ps = (const z_prefix_slice_t *)slice;
    return(ps->prefix_size + ps->data_size);
}

static int __prefix_slice_copy (const z_slice_t *slice,
                                unsigned int offset,
                                void *buffer,
                                unsigned int length)
{
    const z_prefix_slice_t *ps = (const z_prefix_slice_t *)slice;
    unsigned char *pbuf = (unsigned char *)buffer;
    unsigned int n;

    if (offset > (ps->prefix_size + ps->data_size))
        return(0);

    if (offset < ps->prefix_size) {
        if ((n = ps->prefix_size - offset) > length) {
            z_memcpy(pbuf, ps->prefix, length);
            return(length);
        }

        z_memcpy(pbuf, ps->prefix + offset, n);
        length -= n;
        offset = 0;
        pbuf += n;
    }

    n = ps->data_size - offset;
    n = (n < length) ? n : length;
    z_memcpy(pbuf, ps->data + offset, n);
    return((pbuf + n) - ((unsigned char *)buffer));
}

static int __prefix_slice_write (const z_slice_t *slice,
                                 z_stream_t *stream)
{
    const z_prefix_slice_t *ps = (const z_prefix_slice_t *)slice;
    unsigned int np, nd;

    np = z_stream_write_fully(stream, ps->prefix, ps->prefix_size);
    if (np != ps->prefix_size)
        return(np);

    nd = z_stream_write_fully(stream, ps->data, ps->data_size);
    return(np + nd);
}

static int __prefix_slice_rawcmp (const z_slice_t *slice,
                                  unsigned int offset,
                                  const void *blob,
                                  unsigned int length)
{
    /* TODO! */
    return(0);
}

uint8_t __prefix_slice_fetch8 (const z_slice_t *slice, unsigned int offset) {
    const z_prefix_slice_t *ps = (const z_prefix_slice_t *)slice;
    if (offset < ps->prefix_size)
        return(*(ps->data + offset));
    return(*(ps->data + (offset - ps->prefix_size)));
}

static z_slice_vtable_t __prefix_slice = {
    .length  = __prefix_slice_length,
    .copy    = __prefix_slice_copy,
    .write   = __prefix_slice_write,
    .rawcmp  = __prefix_slice_rawcmp,
    .fetch8  = __prefix_slice_fetch8,
    .fetch16 = NULL,
    .fetch32 = NULL,
    .fetch64 = NULL,
};

void z_prefix_slice (z_prefix_slice_t *slice,
                     const void *prefix,
                     unsigned int prefix_size,
                     const void *data,
                     unsigned int data_size)
{
    slice->__base_type__.vtable = &__prefix_slice;
    slice->prefix = prefix;
    slice->data = data;
    slice->prefix_size = prefix_size;
    slice->data_size = data_size;
}
