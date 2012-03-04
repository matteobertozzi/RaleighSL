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

#include <zcl/memset.h>
#include <zcl/memcpy.h>
#include <zcl/memcmp.h>

#include <zcl/buffer.h>

/* =============================================================================
 *  Buffer
 */
static int __buffer_grow (z_buffer_t *buffer,
                          unsigned int size)
{
    unsigned char *blob;

    size = buffer->grow_policy(buffer->user_data, size);
    if ((blob = z_object_blob_realloc(buffer, buffer->blob, size)) == NULL)
        return(-1);

    buffer->block = size;
    buffer->blob = blob;

    return(0);
}

z_buffer_t *z_buffer_alloc (z_buffer_t *buffer,
                            z_memory_t *memory,
                            z_mem_grow_t grow_policy,
                            void *user_data)
{
    if ((buffer = z_object_alloc(memory, buffer, z_buffer_t)) == NULL)
        return(NULL);

    buffer->grow_policy = grow_policy;
    buffer->user_data = user_data;
    buffer->blob = NULL;
    buffer->block = 0U;
    buffer->size = 0U;

    return(buffer);
}

void z_buffer_free (z_buffer_t *buffer) {
    if (buffer->blob != NULL) {
        z_object_blob_free(buffer, buffer->blob);
        buffer->blob = NULL;
    }

    buffer->block = 0U;
    buffer->size = 0U;

    z_object_free(buffer);
}

int z_buffer_squeeze (z_buffer_t *buffer) {
    unsigned int size;
    uint8_t *blob;

    if (buffer->size > 0)
        size = buffer->size;
    else
        size = buffer->grow_policy(buffer->user_data, 0);

    if ((blob = z_object_blob_realloc(buffer, buffer->blob, size)) == NULL)
        return(-1);

    buffer->block = size;
    buffer->blob = blob;

    return(0);
}

int z_buffer_reserve (z_buffer_t *buffer,
                      unsigned int reserve)
{
    if (reserve > buffer->block) {
        if (__buffer_grow(buffer, reserve))
            return(-1);
    }
    return(0);
}

int z_buffer_clear (z_buffer_t *buffer) {
    buffer->size = 0U;
    return(0);
}

int z_buffer_truncate (z_buffer_t *buffer,
                       unsigned int size)
{
    if (size >= buffer->size)
        return(-1);

    buffer->size = size;

    return(0);
}

int z_buffer_remove (z_buffer_t *buffer,
                     unsigned int index,
                     unsigned int size)
{
    if (index >= buffer->size || size == 0)
        return(-1);

    if ((index + size) >= buffer->size) {
        buffer->size = index;
    } else {
        uint8_t *p;

        p = buffer->blob + index;
        z_memmove(p, p + size, buffer->size - index - size);
        buffer->size -= size;
    }

    return(0);
}

int z_buffer_set (z_buffer_t *buffer,
                  const void *blob,
                  unsigned int size)
{
    if (size > buffer->block) {
        if (__buffer_grow(buffer, size))
            return(-1);
    }

    z_memmove(buffer->blob, blob, size);
    buffer->size = size;

    return(0);
}

int z_buffer_append (z_buffer_t *buffer,
                     const void *blob,
                     unsigned int size)
{
    unsigned int n;

    if (Z_UNLIKELY((n = (buffer->size + size)) >= buffer->block)) {
        if (__buffer_grow(buffer, n))
            return(-1);
    }

    z_memcpy(buffer->blob + buffer->size, blob, size);
    buffer->size = n;

    return(0);
}

int z_buffer_prepend (z_buffer_t *buffer,
                      const void *blob,
                      unsigned int size)
{
    unsigned int n;

    if ((n = (buffer->size + size)) > buffer->block) {
        if (__buffer_grow(buffer, n))
            return(-1);
    }

    if ((unsigned char *)blob >= buffer->blob &&
        (unsigned char *)blob < (buffer->blob + buffer->block))
    {
        z_memory_t *memory = z_object_memory(buffer);
        uint8_t *dblob;

        if ((dblob = z_memory_dup(memory, uint8_t, blob, size)) == NULL)
            return(-1);

        z_memmove(buffer->blob + size, buffer->blob, buffer->size);
        z_memcpy(buffer->blob, dblob, size);

        z_memory_free(memory, dblob);
    } else {
        z_memmove(buffer->blob + size, buffer->blob, buffer->size);
        z_memcpy(buffer->blob, blob, size);
    }

    buffer->size += size;

    return(0);
}

int z_buffer_insert (z_buffer_t *buffer,
                     unsigned int index,
                     const void *blob,
                     unsigned int n)
{
    unsigned int size;
    unsigned char *p;

    size = buffer->size + n;
    if (index > buffer->size)
        size += (index - buffer->size);

    if (size > buffer->block) {
        if (__buffer_grow(buffer, size))
            return(-1);
    }

    p = (buffer->blob + index);
    if (index > buffer->size) {
        z_memzero(buffer->blob + buffer->size, index - buffer->size);
        z_memcpy(p, blob, n);
    } else if ((unsigned char *)blob >= buffer->blob &&
               (unsigned char *)blob < (buffer->blob + buffer->block))
    {
        z_memory_t *memory = z_object_memory(buffer);
        uint8_t *dblob;

        if ((dblob = z_memory_dup(memory, uint8_t, blob, n)) == NULL)
            return(-1);

        z_memmove(p + n, p, buffer->size - index);
        z_memcpy(p, dblob, n);

        z_memory_free(memory, dblob);
    } else {
        z_memmove(p + n, p, buffer->size - index);
        z_memcpy(p, blob, n);
    }

    buffer->size = size;

    return(0);
}

int z_buffer_replace (z_buffer_t *buffer,
                      unsigned int index,
                      unsigned int size,
                      const void *blob,
                      unsigned int n)
{
    if (size == n && (index + n) <= buffer->size) {
        z_memmove(buffer->blob + index, blob, n);
    } else {
        z_buffer_remove(buffer, index, size);
        if (z_buffer_insert(buffer, index, blob, n))
            return(-3);
    }
    return(0);
}

int z_buffer_equal (z_buffer_t *buffer,
                    const void *blob,
                    unsigned int size)
{
    if (buffer->size != size)
        return(0);

    return(!z_memcmp(buffer->blob, blob, size));
}

int z_buffer_compare (z_buffer_t *buffer,
                      const void *blob,
                      unsigned int size)
{
    if (buffer->size < size)
        size = buffer->size;
    return(z_memcmp(buffer->blob, blob, size));
}

/* =============================================================================
 *  Buffer Stream
 */
static uint64_t __buffer_tell (z_stream_t *stream) {
    return(((z_buffer_stream_t *)stream)->offset);
}

static int __buffer_seek (z_stream_t *stream, uint64_t offset) {
    z_buffer_stream_t *bstream = (z_buffer_stream_t *)stream;
    if (offset < bstream->buffer->size)
        bstream->offset = (unsigned int)(offset & 0xffffffff);
    else
        bstream->offset = bstream->buffer->size;
    return(0);
}

static int __buffer_read (z_stream_t *stream,
                          void *buffer,
                          unsigned int n)
{
    z_buffer_stream_t *bstream = (z_buffer_stream_t *)stream;
    z_buffer_t *bufobj = bstream->buffer;
    unsigned int size;

    if ((size = (bufobj->size - bstream->offset)) > n)
        size = n;

    z_memcpy(buffer, bufobj->blob, size);
    return(size);
}

static int __buffer_write (z_stream_t *stream,
                           const void *buffer,
                           unsigned int n)
{
    z_buffer_stream_t *bstream = (z_buffer_stream_t *)stream;
    z_buffer_t *bufobj = bstream->buffer;
    unsigned int size;

    if ((size = (bufobj->size - bstream->offset)) > n)
        size = n;

    /* TODO */
    z_memcpy(bufobj->blob + bstream->offset, buffer, size);
    return(size);

}

static z_stream_vtable_t __buffer_stream_vtable = {
    .tell   = __buffer_tell,
    .seek   = __buffer_seek,
    .read   = __buffer_read,
    .write  = __buffer_write,
};

int z_buffer_stream (z_buffer_stream_t *stream,
                     z_buffer_t *buffer)
{
    stream->__base_type__.vtable = &__buffer_stream_vtable;
    stream->buffer = buffer;
    stream->offset = 0;
    return(0);
}

