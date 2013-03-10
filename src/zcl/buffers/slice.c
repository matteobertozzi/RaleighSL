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

#include <zcl/slice.h>

#include <stdio.h>

/* ===========================================================================
 *  PRIVATE Slice Macros
 */
#define __slice_node_blocks_scan(node, block, __ucode__)                    \
    do {                                                                    \
        unsigned int __p_index = Z_SLICE_NBLOCKS;                           \
        block = (node)->blocks;                                             \
        while (__p_index--) {                                               \
            do __ucode__ while (0);                                         \
            block++;                                                        \
        }                                                                   \
    } while (0)

/* ===========================================================================
 *  PRIVATE Slice Utils
 */
static void __slice_node_init (z_slice_node_t *node) {
    z_byteslice_t *bslice;
    node->next = NULL;
    __slice_node_blocks_scan(node, bslice, {
        z_byteslice_clear(bslice);
    });
}

static z_slice_node_t *__slice_node_alloc (z_slice_t *slice) {
    z_slice_node_t *node;

    node = z_object_struct_alloc(slice, z_slice_node_t);
    if (Z_UNLIKELY(node == NULL))
        return(NULL);

    __slice_node_init(node);
    return(node);
}

static void __slice_node_free (z_slice_t *slice, z_slice_node_t *node) {
    z_object_struct_free(slice, z_slice_node_t, node);
}

static z_byteslice_t *__slice_pick_free_buffer (z_slice_node_t *node) {
    z_byteslice_t *bslice;
    __slice_node_blocks_scan(node, bslice, {
        if (z_byteslice_is_empty(bslice))
            return(bslice);
    });
    return(NULL);
}

/* ===========================================================================
 *  PUBLIC Slice methods
 */
void z_slice_clear (z_slice_t *self) {
    z_slice_node_t *node;
    z_slice_node_t *next;

    for (node = self->head.next; node != NULL; node = next) {
        next = node->next;
        __slice_node_free(self, node);
    }

    self->tail = &(self->head);
    __slice_node_init(self->tail);
}

int z_slice_add (z_slice_t *self, const void *data, size_t size) {
    z_byteslice_t *bslice;

    if ((bslice = __slice_pick_free_buffer(self->tail)) == NULL) {
        z_slice_node_t *node;

        if ((node = __slice_node_alloc(self)) == NULL)
            return(-1);

        self->tail->next = node;
        self->tail = node;
        bslice = &(node->blocks[0]);
    }

    z_byteslice_set(bslice, data, size);
    return(0);
}

size_t z_slice_length (const z_slice_t *self) {
    const z_byteslice_t *bslice;
    const z_slice_node_t *node;
    size_t length = 0;

    for (node = &(self->head); node != NULL; node = node->next) {
        __slice_node_blocks_scan(node, bslice, {
            if (bslice->length == 0)
                return(length);
            length += bslice->length;
        });
    }

    return(length);
}

int z_slice_is_empty (const z_slice_t *self) {
    return(z_byteslice_is_empty(&(self->head.blocks[0])));
}

int z_slice_equals (z_slice_t *self, const void *data, size_t size) {
    const uint8_t *pdata = (const uint8_t *)data;
    const z_byteslice_t *bslice;
    const z_slice_node_t *node;

    for (node = &(self->head); node != NULL; node = node->next) {
        __slice_node_blocks_scan(node, bslice, {
            size_t n;

            if (size < bslice->length)
                return(0);

            if ((n = z_min(size, bslice->length)) == 0)
                return(size == 0);

            if (!z_byteslice_starts_with(bslice, pdata, n))
                return(0);

            pdata += n;
            size -= n;
        });
    }

    return(size == 0);
}

int z_slice_starts_with (z_slice_t *self, const void *data, size_t size) {
    const uint8_t *pdata = (const uint8_t *)data;
    const z_byteslice_t *bslice;
    const z_slice_node_t *node;

    for (node = &(self->head); node != NULL; node = node->next) {
        __slice_node_blocks_scan(node, bslice, {
            size_t n;

            if ((n = z_min(size, bslice->length)) == 0)
                return(size == 0);

            if (!z_byteslice_starts_with(bslice, pdata, size))
                return(0);

            pdata += n;
            size -= n;
        });
    }

    return(size == 0);
}

size_t z_slice_dump (const z_slice_t *self) {
    const z_byteslice_t *bslice;
    const z_slice_node_t *node;
    size_t length = 0;
    int i, j;

    for (node = &(self->head); node != NULL; node = node->next) {
        for (i = 0; i < Z_SLICE_NBLOCKS; ++i) {
            bslice = &(node->blocks[i]);
            if (bslice->length == 0) {
                fputc('\n', stdout);
                return(length);
            }

            length += bslice->length;
            for (j = 0; j < bslice->length; ++j) {
                fputc(bslice->data[j], stdout);
            }
        }
    }
    fputc('\n', stdout);
    return(length);
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __slice_ctor (void *self, z_memory_t *memory, va_list args) {
    z_slice_t *slice = Z_SLICE(self);
    slice->tail = &(slice->head);
    __slice_node_init(slice->tail);
    return(0);
}

static void __slice_dtor (void *self) {
    z_slice_clear(Z_SLICE(self));
}

/* ===========================================================================
 *  INTERFACE Reader methods
 */
static int __slice_reader_open (void *self, const void *object) {
    z_slice_reader_t *reader = Z_SLICE_READER(self);
    const z_slice_t *slice = Z_CONST_SLICE(object);
    Z_READER_INIT(reader, slice);
    reader->node = &(slice->head);
    reader->iblock = 0;
    reader->offset = 0;
    return(0);
}

static void __slice_reader_close (void *self) {
}

static size_t __slice_reader_next (void *self, uint8_t **data) {
    z_slice_reader_t *reader = Z_SLICE_READER(self);
    const z_slice_node_t *node = reader->node;
    const z_byteslice_t *block = &(node->blocks[reader->iblock]);
    size_t n;

    if (node == NULL || z_byteslice_is_empty(block))
        return(0);

    /* still some bytes left */
    if (reader->offset < block->length) {
        *data = block->data + reader->offset;
        n = block->length - reader->offset;
        reader->offset = block->length;
        return(n);
    }

    if (reader->iblock++ < Z_SLICE_NBLOCKS) {
        block = &(node->blocks[reader->iblock]);
    } else {
        if ((node = node->next) == NULL)
            return(0);

        reader->node = node;
        reader->iblock = 0;
        block = &(node->blocks[0]);
    }

    if (z_byteslice_is_empty(block))
        return(0);

    reader->offset = block->length;
    *data = block->data;
    return(block->length);
}

static void __slice_reader_backup (void *self, size_t count) {
    z_slice_reader_t *reader = Z_SLICE_READER(self);
    reader->offset -= count;
}

/* ===========================================================================
 *  Slice vtables
 */
static const z_vtable_type_t __slice_type = {
    .name = "Slice",
    .size = sizeof(z_slice_t),
    .ctor = __slice_ctor,
    .dtor = __slice_dtor,
};

static const z_vtable_reader_t __slice_reader = {
    .open       = __slice_reader_open,
    .close      = __slice_reader_close,
    .next       = __slice_reader_next,
    .backup     = __slice_reader_backup,
    .available  = NULL,
};

static const z_slice_interfaces_t __slice_interfaces = {
    .type   = &__slice_type,
    .reader = &__slice_reader,
};

/* ===========================================================================
 *  PUBLIC Slice constructor/destructor
 */
z_slice_t *z_slice_alloc (z_slice_t *self, z_memory_t *memory) {
    return(z_object_alloc(self, &__slice_interfaces, memory));
}

void z_slice_free (z_slice_t *self) {
    z_object_free(self);
}
