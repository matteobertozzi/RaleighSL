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

#include <zcl/chunkq.h>
#include <zcl/stream.h>
#include <zcl/memcmp.h>
#include <zcl/memcpy.h>
#include <zcl/rdata.h>

/* ===========================================================================
 *  Private Methods
 */
#define __plug_has_method(rdata, method)                                    \
    ((rdata)->plug->method != NULL)

#define __plug_call(rdata, method, ...)                                     \
    ((rdata)->plug->method(rdata, ##__VA_ARGS__))

#define __is_plug(rdata, type)                                              \
    ((rdata)->plug == &(type))

/* ===========================================================================
 *  Blob plugin
 */
static unsigned int __blob_length (const z_rdata_t *rdata) {
    return(rdata->object.blob.length);
}

static unsigned int __blob_read (const z_rdata_t *rdata,
                                 unsigned int offset,
                                 void *buffer,
                                 unsigned int size)
{
    z_memcpy(buffer, rdata->object.blob.data + offset, size);
    return(size);
}

static unsigned int __blob_fetch (const z_rdata_t *rdata,
                                  unsigned int offset,
                                  unsigned int size,
                                  z_iopush_t func,
                                  void *user_data)
{
    return(func(user_data, rdata->object.blob.data + offset, size));
}

static z_rdata_plug_t __rdata_blob_plug = {
    .length  = __blob_length,
    .read    = __blob_read,
    .fetch   = __blob_fetch,
    .memcmp  = NULL,
    .rawcmp  = NULL,
};

/* ===========================================================================
 *  Stream plugin
 */
static unsigned int __stream_length (const z_rdata_t *rdata) {
    return(Z_CONST_STREAM_EXTENT(rdata->object.custom)->length);
}

static unsigned int __stream_read (const z_rdata_t *rdata,
                                   unsigned int offset,
                                   void *buffer,
                                   unsigned int size)
{
    const z_stream_extent_t *e = Z_CONST_STREAM_EXTENT(rdata->object.custom);
    return(z_stream_pread(e->stream, e->offset + offset, buffer, size));
}

static unsigned int __stream_fetch (const z_rdata_t *rdata,
                                    unsigned int offset,
                                    unsigned int n,
                                    z_iopush_t func,
                                    void *user_data)
{
    const z_stream_extent_t *e = Z_CONST_STREAM_EXTENT(rdata->object.custom);
    return(z_stream_fetch(e->stream, e->offset + offset, n, func, user_data));
}

static int __stream_rawcmp (const z_rdata_t *rdata,
                            unsigned int offset,
                            const void *data,
                            unsigned int length)
{
    const z_stream_extent_t *e = Z_CONST_STREAM_EXTENT(rdata->object.custom);
    return(z_stream_memcmp(e->stream, e->offset + offset, data, length));
}

static z_rdata_plug_t __rdata_stream_plug = {
    .length = __stream_length,
    .read   = __stream_read,
    .fetch  = __stream_fetch,
    .memcmp = NULL,
    .rawcmp = __stream_rawcmp,
};

/* ===========================================================================
 *  Chunkq plugin
 */
static unsigned int __chunkq_length (const z_rdata_t *rdata) {
    return(Z_CONST_CHUNKQ_EXTENT(rdata->object.custom)->length);
}

static unsigned int __chunkq_read (const z_rdata_t *rdata,
                                   unsigned int offset,
                                   void *buffer,
                                   unsigned int size)
{
    const z_chunkq_extent_t *e = Z_CONST_CHUNKQ_EXTENT(rdata->object.custom);
    return(z_chunkq_read(e->chunkq, e->offset + offset, buffer, size));
}

static unsigned int __chunkq_fetch (const z_rdata_t *rdata,
                                    unsigned int offset,
                                    unsigned int n,
                                    z_iopush_t func,
                                    void *user_data)
{
    const z_chunkq_extent_t *e = Z_CONST_CHUNKQ_EXTENT(rdata->object.custom);
    return(z_chunkq_fetch(e->chunkq, e->offset + offset, n, func, user_data));
}

static int __chunkq_rawcmp (const z_rdata_t *rdata,
                            unsigned int offset,
                            const void *data,
                            unsigned int length)
{
    const z_chunkq_extent_t *e = Z_CONST_CHUNKQ_EXTENT(rdata->object.custom);
    return(z_chunkq_memcmp(e->chunkq, e->offset + offset, data, length));
}

static z_rdata_plug_t __rdata_chunkq_plug = {
    .length = __chunkq_length,
    .read   = __chunkq_read,
    .fetch  = __chunkq_fetch,
    .memcmp = NULL,
    .rawcmp = __chunkq_rawcmp,
};

/* ===========================================================================
 *  Public Methods
 */
z_rdata_t *z_rdata_from_plug (z_rdata_t *rdata,
                              z_rdata_plug_t *plug,
                              const void *object)
{
    rdata->object.custom = object;
    rdata->plug = plug;
    return(rdata);
}

z_rdata_t *z_rdata_from_blob (z_rdata_t *rdata,
                              const void *data,
                              unsigned int length)
{
    rdata->object.blob.data = (unsigned char *)data;
    rdata->object.blob.length = length;
    rdata->plug = &__rdata_blob_plug;
    return(rdata);
}

z_rdata_t *z_rdata_from_stream (z_rdata_t *rdata,
                                const void *stream)
{
    rdata->object.custom = stream;
    rdata->plug = &__rdata_stream_plug;
    return(rdata);
}

z_rdata_t *z_rdata_from_chunkq (z_rdata_t *rdata,
                                const void *chunkq)
{
    rdata->object.custom = chunkq;
    rdata->plug = &__rdata_chunkq_plug;
    return(rdata);
}

unsigned int z_rdata_length (const z_rdata_t *rdata) {
    return(__plug_call(rdata, length));
}

unsigned int z_rdata_read (const z_rdata_t *rdata,
                           unsigned int offset,
                           void *buffer,
                           unsigned int size)
{
    return(__plug_call(rdata, read, offset, buffer, size));
}

unsigned int z_rdata_readc (const z_rdata_t *rdata,
                            unsigned int offset,
                            void *buffer)
{
    return(__plug_call(rdata, read, offset, buffer, 1));
}

unsigned int z_rdata_fetch (const z_rdata_t *rdata,
                            unsigned int offset,
                            unsigned int size,
                            z_iopush_t func,
                            void *user_data)
{
    return(__plug_call(rdata, fetch, offset, size, func, user_data));
}

unsigned int z_rdata_fetch_all (const z_rdata_t *rdata,
                                z_iopush_t func,
                                void *user_data)
{
    return(z_rdata_fetch(rdata, 0, z_rdata_length(rdata), func, user_data));
}

int z_rdata_rawcmp (const z_rdata_t *rdata,
                    unsigned int offset,
                    const void *data,
                    unsigned int length)
{
    if (__plug_has_method(rdata, rawcmp))
        return(__plug_call(rdata, rawcmp, offset, data, length));

    if (__is_plug(rdata, __rdata_blob_plug)) {
        return(z_memcmp(rdata->object.blob.data + offset, data, length));
    } else {
        z_rdata_t xdata;
        z_rdata_from_blob(&xdata, data, length);
        return(__plug_call(rdata, memcmp, offset, &xdata, 0, length));
    }

    return(0);
}

int z_rdata_memcmp (const z_rdata_t *rdata1,
                    unsigned int offset1,
                    const z_rdata_t *rdata2,
                    unsigned int offset2,
                    unsigned int length)
{
    unsigned char c1, c2;
    unsigned int l, l1, l2;

    if (__plug_has_method(rdata1, memcmp))
        return(__plug_call(rdata1, memcmp, offset1, rdata2, offset2, length));

    l1 = z_rdata_length(rdata1) - offset1;
    l2 = z_rdata_length(rdata2) - offset2;
    l = z_min(z_min(l1, l2), length);

    while (l--) {
        z_rdata_readc(rdata1, offset1++, &c1);
        z_rdata_readc(rdata2, offset2++, &c2);

        if (c1 != c2)
            return(c1 - c2);
    }

    return((length <= l) ? 0 : (l1 - l2));
}

const void *z_rdata_blob (const z_rdata_t *rdata) {
    if (__is_plug(rdata, __rdata_blob_plug))
        return(rdata->object.blob.data);
    return(NULL);
}


void *z_rdata_to_bytes (const z_rdata_t *rdata, z_memory_t *memory) {
    unsigned int length;
    void *bytes;

    length = z_rdata_length(rdata);
    if ((bytes = z_memory_alloc(memory, length)) != NULL)
        z_rdata_read(rdata, 0, bytes, length);

    return(bytes);
}

