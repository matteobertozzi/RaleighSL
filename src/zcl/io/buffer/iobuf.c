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
#include <zcl/iobuf.h>
#include <zcl/debug.h>

/* ===========================================================================
 *  PRIVATE iobuf data structures
 */
struct z_iobuf_node {
    z_iobuf_node_t *next;
    unsigned char data[0];
};

/* ===========================================================================
 *  PRIVATE I/O Buf macros
 */
#define __iobuf_block(iobuf)                                                \
    z_object_flags(iobuf).uflags32

#define __iobuf_set_block(iobuf, block)                                     \
    z_object_flags(iobuf).uflags32 = z_align_up(block, 8)

#define __iobuf_total_size(iobuf)                                           \
    ((iobuf)->offset + (iobuf)->length)

#define __iobuf_last_block_size(iobuf)                                      \
    (__iobuf_total_size(iobuf) & (__iobuf_block(iobuf) - 1))

/* ===========================================================================
 *  PRIVATE iobuf methods
 */
static z_iobuf_node_t *__iobuf_node_alloc (z_iobuf_t *iobuf) {
    z_iobuf_node_t *node;
    unsigned int size;

    size = sizeof(z_iobuf_node_t) + __iobuf_block(iobuf);
    if ((node = z_object_block_alloc(iobuf, z_iobuf_node_t, size)) != NULL) {
        node->next = NULL;
    }
    return(node);
}

static void __iobuf_node_free (z_iobuf_t *iobuf, z_iobuf_node_t *node) {
    z_object_block_free(iobuf, node);
}

/* ===========================================================================
 *  PUBLIC IOBuf methods
 */
void z_iobuf_clear (z_iobuf_t *iobuf) {
    z_iobuf_node_t *node;

    while ((node = iobuf->head) != NULL) {
        iobuf->head = node->next;
        __iobuf_node_free(iobuf, node);
    }

    iobuf->tail = NULL;
    iobuf->offset = 0;
    iobuf->length = 0;
}

void z_iobuf_drain (z_iobuf_t *iobuf, unsigned int n) {
    if (iobuf->head == iobuf->tail) {
        if (n < iobuf->length) {
            iobuf->offset += n;
            iobuf->length -= n;
        } else {
            __iobuf_node_free(iobuf, iobuf->head);
            iobuf->length = 0;
            iobuf->offset = 0;
            iobuf->head = NULL;
            iobuf->tail = NULL;
        }
    } else {
        unsigned char *data;
        ssize_t rd;

        while ((rd = z_iobuf_read(iobuf, &data)) > 0) {
            if (n <= rd) {
                z_iobuf_read_backup(iobuf, rd - n);
                break;
            }
            n -= rd;
        }

        if (iobuf->head != NULL) {
            __iobuf_node_free(iobuf, iobuf->head);
            iobuf->head = NULL;
            iobuf->tail = NULL;
            iobuf->offset = 0;
        }
    }
}

ssize_t z_iobuf_read (z_iobuf_t *iobuf, unsigned char **data) {
    z_iobuf_node_t *node = iobuf->head;
    ssize_t n;

    if (Z_LIKELY(iobuf->length > 0)) {
        if (iobuf->offset == __iobuf_block(iobuf)) {
            node = node->next;
            *data = node->data;
            n = __iobuf_block(iobuf);
            n = z_min(n, iobuf->length);
            iobuf->offset = n;

            __iobuf_node_free(iobuf, iobuf->head);
            iobuf->head = node;
        } else {
            *data = node->data + iobuf->offset;
            n = __iobuf_block(iobuf) - iobuf->offset;
            n = z_min(n, iobuf->length);
            iobuf->offset += n;
        }

        iobuf->length -= n;
        return(n);
    }

    if (node != NULL) {
        __iobuf_node_free(iobuf, iobuf->head);
        iobuf->head = NULL;
        iobuf->tail = NULL;
        iobuf->offset = 0;
    }

    return(0);
}

void z_iobuf_read_backup (z_iobuf_t *iobuf, ssize_t n) {
    iobuf->offset -= n;
    iobuf->length += n;
}

ssize_t z_iobuf_write (z_iobuf_t *iobuf, unsigned char **data) {
    z_iobuf_node_t *node = iobuf->tail;
    ssize_t n;

    n = __iobuf_last_block_size(iobuf);
    /* TODO: CHECK ME EMPTY BLOCK? */
    if (n == 0) {
        if ((node = __iobuf_node_alloc(iobuf)) == NULL)
            return(0);

        if (iobuf->tail == NULL)
            iobuf->head = node;
        else
            iobuf->tail->next = node;
        iobuf->tail = node;
        iobuf->length += __iobuf_block(iobuf);

        *data = node->data;
        return(__iobuf_block(iobuf));
    }

    *data = node->data + n;
    n = __iobuf_block(iobuf) - n;
    iobuf->length += n;
    return(n);
}

void z_iobuf_write_backup (z_iobuf_t *iobuf, ssize_t n) {
    if (n == __iobuf_block(iobuf)) {
        /* TODO unsafe: keep the address of the prev in the tail data */
        z_iobuf_node_t *p = iobuf->head;
        while (p->next != iobuf->tail)
            p = p->next;
        __iobuf_node_free(iobuf, iobuf->tail);
        p->next = NULL;
        iobuf->tail = p;
    }
    iobuf->length -= n;
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __iobuf_ctor (void *self, z_memory_t *memory, va_list args) {
    unsigned int block = va_arg(args, unsigned int);
    z_iobuf_t *iobuf = Z_IOBUF(self);
    __iobuf_set_block(iobuf, block);
    iobuf->head = NULL;
    iobuf->tail = NULL;
    iobuf->offset = 0;
    iobuf->length = 0;
    return(0);
}

static void __iobuf_dtor (void *self) {
    z_iobuf_clear(self);
}

/* ===========================================================================
 *  INTERFACE Reader methods
 */
static int __iobuf_reader_open (void *self, const void *object) {
    z_iobuf_reader_t *reader = Z_IOBUF_READER(self);
    const z_iobuf_t *iobuf = Z_CONST_IOBUF(object);
    Z_READER_INIT(reader, iobuf);
    reader->node = iobuf->head;
    reader->available = iobuf->length;
    reader->offset = iobuf->offset;
    return(0);
}

static void __iobuf_reader_close (void *self) {
}

static size_t __iobuf_reader_next (void *self, uint8_t **data) {
    const z_iobuf_t *iobuf = Z_READER_READABLE(z_iobuf_t, self);
    z_iobuf_reader_t *reader = Z_IOBUF_READER(self);
    z_iobuf_node_t *node = reader->node;
    size_t n;

    if (node == NULL || reader->available == 0)
        return(0);

    n = (node->next != NULL) ? __iobuf_block(iobuf) : __iobuf_last_block_size(iobuf);
    if (reader->offset < n) {
        *data = node->data + reader->offset;
        n -= reader->offset;
        n = z_min(n, reader->available);
        reader->available -= n;
        reader->offset += n;
        return(n);
    }

    if ((node = node->next) == NULL)
        return(0);

    n = z_min(n, reader->available);
    reader->node = node;
    reader->available -= n;
    reader->offset = n;
    *data = node->data;
    return(n);
}

static void __iobuf_reader_backup (void *self, size_t count) {
    z_iobuf_reader_t *reader = Z_IOBUF_READER(self);
    reader->available += count;
    reader->offset -= count;
}

static size_t __iobuf_reader_available (void *self) {
    return(Z_IOBUF_READER(self)->available);
}

/* ===========================================================================
 *  I/O Buf vtables
 */
static const z_vtable_type_t __iobuf_type = {
    .name = "IOBuf",
    .size = sizeof(z_iobuf_t),
    .ctor = __iobuf_ctor,
    .dtor = __iobuf_dtor,
};

static const z_vtable_reader_t __iobuf_reader = {
    .open       = __iobuf_reader_open,
    .close      = __iobuf_reader_close,
    .next       = __iobuf_reader_next,
    .backup     = __iobuf_reader_backup,
    .available  = __iobuf_reader_available,
};

static const z_iobuf_interfaces_t __iobuf_interfaces = {
    .type       = &__iobuf_type,
    .reader     = &__iobuf_reader,
};

/* ===========================================================================
 *  PUBLIC Skip-List constructor/destructor
 */
z_iobuf_t *z_iobuf_alloc (z_iobuf_t *self,
                          z_memory_t *memory,
                          unsigned int block)
{
    return(z_object_alloc(self, &__iobuf_interfaces, memory, block));
}

void z_iobuf_free (z_iobuf_t *self) {
    z_object_free(self);
}

