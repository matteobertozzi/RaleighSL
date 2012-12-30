/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

#include <zcl/string.h>
#include <zcl/bytes.h>

/* ===========================================================================
 *  PRIVATE Byte Array utils
 */
static int __bytes_grow (z_bytes_t *self, size_t size) {
    uint8_t *block;

#if 0
    size = self->grow_policy(self->user_data, size);
#else
    size = size + ((size < 1024) ? 32 : 1024);
#endif

    size = z_align_up(size, 8);
    block = z_object_block_realloc(self, uint8_t, self->block, size);
    if (Z_UNLIKELY(block == NULL))
        return(-1);

    self->blksize = size;
    self->block = block;

    return(0);
}

/* ===========================================================================
 *  PUBLIC Byte Array
 */
int z_bytes_squeeze (z_bytes_t *self) {
    uint8_t *block;
    size_t size;

    if (self->size > 0) {
        size = self->size;
    } else {
#if 0
        size = self->grow_policy(self->user_data, 0);
#else
        size = 64;
#endif
    }

    block = z_object_block_realloc(self, uint8_t, self->block, size);
    if (Z_UNLIKELY(block == NULL))
        return(-1);

    self->blksize = size;
    self->block = block;
    return(0);
}

int z_bytes_reserve (z_bytes_t *self, size_t reserve) {
    if (reserve > self->blksize) {
        return(__bytes_grow(self, reserve));
    }
    return(0);
}

int z_bytes_clear (z_bytes_t *self) {
    self->size = 0U;
    return(0);
}

int z_bytes_truncate (z_bytes_t *self, size_t size) {
    if (size >= self->size)
        return(-1);

    self->size = size;
    return(0);
}

int z_bytes_remove (z_bytes_t *self, size_t index, size_t size) {
    if (index >= self->size || size == 0)
        return(-1);

    if ((index + size) >= self->size) {
        self->size = index;
    } else {
        uint8_t *p;

        p = self->block + index;
        z_memmove(p, p + size, self->size - index - size);
        self->size -= size;
    }

    return(0);
}

int z_bytes_set (z_bytes_t *self, const void *blob, size_t size) {
    if (size > self->blksize) {
        if (__bytes_grow(self, size))
            return(-1);
    }

    z_memmove(self->block, blob, size);
    self->size = size;
    return(0);
}

int z_bytes_append (z_bytes_t *self, const void *blob, size_t size) {
    size_t n;

    if (Z_UNLIKELY((n = (self->size + size)) >= self->blksize)) {
        if (__bytes_grow(self, n))
            return(-1);
    }

    z_memcpy(self->block + self->size, blob, size);
    self->size = n;
    return(0);
}

int z_bytes_prepend (z_bytes_t *self, const void *blob, size_t size) {
    size_t n;

    if ((n = (self->size + size)) > self->blksize) {
        if (__bytes_grow(self, n))
            return(-1);
    }

    if ((uint8_t *)blob >= self->block &&
        (uint8_t *)blob < (self->block + self->blksize))
    {
        z_memory_t *memory = z_object_memory(self);
        uint8_t *dblob;

        if ((dblob = z_memory_dup(memory, uint8_t, blob, size)) == NULL)
            return(-1);

        z_memmove(self->block + size, self->block, self->size);
        z_memcpy(self->block, dblob, size);

        z_memory_free(memory, dblob);
    } else {
        z_memmove(self->block + size, self->block, self->size);
        z_memcpy(self->block, blob, size);
    }

    self->size += size;
    return(0);
}

int z_bytes_insert (z_bytes_t *self, size_t index, const void *blob, size_t n)
{
    size_t size;
    uint8_t *p;

    size = self->size + n;
    if (index > self->size)
        size += (index - self->size);

    if (size > self->blksize) {
        if (__bytes_grow(self, size))
            return(-1);
    }

    p = (self->block + index);
    if (index > self->size) {
        z_memzero(self->block + self->size, index - self->size);
        z_memcpy(p, blob, n);
    } else if ((uint8_t *)blob >= self->block &&
               (uint8_t *)blob < (self->block + self->blksize))
    {
        z_memory_t *memory = z_object_memory(self);
        uint8_t *dblob;

        if ((dblob = z_memory_dup(memory, uint8_t, blob, n)) == NULL)
            return(-1);

        z_memmove(p + n, p, self->size - index);
        z_memcpy(p, dblob, n);

        z_memory_free(memory, dblob);
    } else {
        z_memmove(p + n, p, self->size - index);
        z_memcpy(p, blob, n);
    }

    self->size = size;
    return(0);
}

int z_bytes_replace (z_bytes_t *self,
                     size_t index,
                     size_t size,
                     const void *blob,
                     size_t n)
{
    if (size == n && (index + n) <= self->size) {
        z_memmove(self->block + index, blob, n);
    } else {
        z_bytes_remove(self, index, size);
        if (z_bytes_insert(self, index, blob, n))
            return(-3);
    }
    return(0);
}

int z_bytes_equal (z_bytes_t *self, const void *blob, size_t size) {
    if (self->size != size)
        return(0);

    return(!z_memcmp(self->block, blob, size));
}

int z_bytes_compare (z_bytes_t *self, const void *blob, size_t size) {
    if (self->size < size)
        size = self->size;
    return(z_memcmp(self->block, blob, size));
}


/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __bytes_ctor (void *self, z_memory_t *memory, va_list args) {
    z_bytes_t *bytes = Z_BYTES(self);
    bytes->block = NULL;
    bytes->blksize = 0U;
    bytes->size = 0U;
    return(0);
}

static void __bytes_dtor (void *self) {
    z_bytes_t *bytes = Z_BYTES(self);

    if (bytes->block != NULL) {
        z_object_block_free(self, bytes->block);
        bytes->block = NULL;
    }

    bytes->blksize = 0U;
    bytes->size = 0U;

    z_bytes_clear(bytes);
}

/* ===========================================================================
 *  Bytes vtables
 */
static const z_vtable_type_t __bytes_type = {
    .name = "Bytes",
    .size = sizeof(z_bytes_t),
    .ctor = __bytes_ctor,
    .dtor = __bytes_dtor,
};

static const z_bytes_interfaces_t __bytes_interfaces = {
    .type = &__bytes_type,
};

/* ===========================================================================
 *  PUBLIC Bytes constructor/destructor
 */
z_bytes_t *z_bytes_alloc (z_bytes_t *self, z_memory_t *memory) {
    return(z_object_alloc(self, &__bytes_interfaces, memory));
}

void z_bytes_free (z_bytes_t *self) {
    z_object_free(self);
}
