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

#include <unistd.h>

#include <zcl/atomic.h>
#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/ipc.h>

/*
  +----------+-------+---------+  +---------+--------+---+
  | msg 1    | msg 2 | msg 3.0 |  | msg 3.1 | msg 4  |   |
  +----------+-------+---------+  +---------+--------+---+
 */

/* ===========================================================================
 *  PRIVATE msgbuf data structures
 */
struct z_ipc_msgbuf_node {
    volatile unsigned int refs;
    unsigned int available;
    z_ipc_msgbuf_node_t *next;
    uint8_t data[0];
};

/* ===========================================================================
 *  PRIVATE msgbuf macros
 */
#define __MSGBUF_SIZE                        128
#define __MSGBUF_MIN_READ                   (8)

#define __msgbuf_node_used(node)            (__MSGBUF_SIZE - (node)->available)

/* ===========================================================================
 *  PRIVATE msgbuf methods
 */
static z_ipc_msgbuf_node_t *__msgbuf_node_alloc (z_ipc_msgbuf_t *msgbuf) {
  z_ipc_msgbuf_node_t *node;
  unsigned int node_size;

  node_size = sizeof(z_ipc_msgbuf_node_t) + __MSGBUF_SIZE;
  if ((node = z_memory_alloc(msgbuf->memory, z_ipc_msgbuf_node_t, node_size))) {
    node->next = NULL;
    node->available = __MSGBUF_SIZE;
    node->refs = 1;
    msgbuf->refs++;
  }

  return(node);
}

static void __msgbuf_node_free (z_ipc_msgbuf_t *msgbuf, z_ipc_msgbuf_node_t *node) {
    if (!z_atomic_dec(&(node->refs))) {
        z_memory_free(msgbuf->memory, node);
        msgbuf->refs--;
    }
}

static void __msg_release (z_ipc_msg_t *msg) {
    z_ipc_msgbuf_node_t *node = msg->head;
    int blocks = msg->blocks;

    blocks = msg->blocks;
    while (blocks--) {
        z_ipc_msgbuf_node_t *next = node->next;
        __msgbuf_node_free(msg->msgbuf, node);
        node = next;
    }
}

/* ===========================================================================
 *  PUBLIC msgbuf methods
 */
void z_ipc_msgbuf_open (z_ipc_msgbuf_t *msgbuf, z_memory_t *memory) {
    msgbuf->memory = memory;
    msgbuf->head = NULL;
    msgbuf->tail = NULL;
    msgbuf->length = 0;
    msgbuf->offset = 0;
}

void z_ipc_msgbuf_close (z_ipc_msgbuf_t *msgbuf) {
    z_ipc_msgbuf_clear(msgbuf);
}

void z_ipc_msgbuf_clear (z_ipc_msgbuf_t *msgbuf) {
    z_ipc_msgbuf_node_t *node;
    z_ipc_msgbuf_node_t *next;
    for (node = msgbuf->head; node != NULL; node = next) {
        next = node->next;
        __msgbuf_node_free(msgbuf, node);
    }
    msgbuf->head = NULL;
    msgbuf->tail = NULL;
    msgbuf->length = 0;
    msgbuf->offset = 0;
}

int z_ipc_msgbuf_add (z_ipc_msgbuf_t *msgbuf, int fd) {
    z_ipc_msgbuf_node_t *node;
    ssize_t rd;
    int size;

    if ((node = msgbuf->tail) == NULL) {
        /* empty buffer, allocate the first block */
        if ((node = __msgbuf_node_alloc(msgbuf)) == NULL)
            return(-1);

        msgbuf->head = node;
        msgbuf->tail = node;
    } else if (node == msgbuf->head && msgbuf->offset > 0 &&
               z_atomic_get(&(node->refs)) == 0)
    {
        /* repack the node */
        z_memmove(node->data, node->data + msgbuf->offset, __msgbuf_node_used(node));
        node->available += msgbuf->offset;
        msgbuf->offset = 0;
    }

    /* read() is too wasteful here, just allocate another node */
    if (node->available < __MSGBUF_MIN_READ) {
        if ((node = __msgbuf_node_alloc(msgbuf)) == NULL)
            return(-2);

        msgbuf->tail->next = node;
        msgbuf->tail = node;
    }

    /* try to fill the block */
    if ((rd = read(fd, node->data + __msgbuf_node_used(node), node->available)) < 0) {
        perror("read()");
        return(-3);
    }

    size = (rd & 0xffffffff);
    node->available -= size;
    msgbuf->length += size;
    return(size);
}

z_ipc_msg_t *z_ipc_msgbuf_get (z_ipc_msgbuf_t *msgbuf, z_ipc_msg_t *msg) {
    z_ipc_msgbuf_node_t *node;
    unsigned int shift = 0;
    unsigned int count = 1;
    uint64_t size = 0;
    uint32_t offset;

    node = msgbuf->head;
    offset = msgbuf->offset;
    while (node != NULL) {
        size_t node_size = __msgbuf_node_used(node);
        while (offset < node_size) {
            uint64_t b = node->data[offset++];
            if (b & 128) {
                size |= (b & 0x7f) << shift;
            } else {
                size |= (b << shift);
                // TODO: Cache it!
                return(z_ipc_msgbuf_get_at(msgbuf, msg, count, size));
            }
            shift += 7;
            count++;
        }
        node = node->next;
        offset = 0;
    }

    return(NULL);
}

z_ipc_msg_t *z_ipc_msgbuf_get_at (z_ipc_msgbuf_t *msgbuf,
                                  z_ipc_msg_t *msg,
                                  uint32_t offset,
                                  uint64_t length)
{
    z_ipc_msgbuf_node_t *node;
    unsigned int node_size;

    /* Nothing in here, sorry */
    if ((offset + length) > msgbuf->length)
        return(NULL);

    node = msgbuf->head;
    if ((node_size = (__msgbuf_node_used(node) - msgbuf->offset)) == 0)
        return(NULL);

    /* Skip tag nodes */
    while (offset >= node_size) {
        z_ipc_msgbuf_node_t *head = node;

        /* first node can be skipped, no data here */
        node = node->next;
        __msgbuf_node_free(msgbuf, head);
        msgbuf->head = node;
        msgbuf->offset = 0;

        offset -= node_size;
        node_size = (node != NULL) ? __msgbuf_node_used(node) : 1;
    }

    /* Update msgbuf size */
    node_size -= offset;
    msgbuf->length -= (offset + length);
    msgbuf->offset += offset;

    if (length == 0) {
        fprintf(stderr, "WARN: Message with no content\n");
        return(NULL);
    }

    /* Initialize the message references */
    if ((msg = z_ipc_msg_alloc(msg, msgbuf, msgbuf->offset, length)) == NULL)
        return(NULL);

    /* Keep a references to the chunks that compose the message */
    do {
        z_ipc_msg_add_node(msg, node);

        if (length <= node_size) {
            msgbuf->offset += length;
            break;
        }

        /* switch head */
        node = node->next;
        __msgbuf_node_free(msgbuf, msgbuf->head);
        msgbuf->head = node;
        msgbuf->offset = 0;

        length -= node_size;
        node_size = __msgbuf_node_used(node);
    } while (1);

    /* Cleanup if the node is not useful */
    if ((msgbuf->offset + node->available) == __MSGBUF_SIZE) {
        z_ipc_msgbuf_node_t *head = node;

        if (msgbuf->head == msgbuf->tail) {
            msgbuf->head = NULL;
            msgbuf->tail = NULL;
        } else {
            msgbuf->head = node->next;
        }

        msgbuf->offset = 0;
        __msgbuf_node_free(msgbuf, head);
    }

    return(msg);
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __ipc_msg_ctor (void *self, z_memory_t *memory, va_list args) {
    z_ipc_msg_t *msg = Z_IPC_MSG(self);
    msg->msgbuf = va_arg(args, z_ipc_msgbuf_t *);
    msg->head = msg->msgbuf->head;
    msg->offset = va_arg(args, uint32_t);
    msg->length = va_arg(args, uint64_t);
    msg->blocks = 0;
    return(0);
}

static void __ipc_msg_dtor (void *self) {
    __msg_release(Z_IPC_MSG(self));
}

/* ===========================================================================
 *  INTERFACE Reader methods
 */
static int __ipc_msg_reader_open (void *self, const void *object) {
    z_ipc_msg_reader_t *reader = Z_IPC_MSG_READER(self);
    const z_ipc_msg_t *ipc_msg = Z_CONST_IPC_MSG(object);
    Z_READER_INIT(reader, ipc_msg);
    reader->node = ipc_msg->head;
    reader->available = ipc_msg->length;
    reader->offset = 0;
    return(0);
}

static void __ipc_msg_reader_close (void *self) {
}

static size_t __ipc_msg_reader_next (void *self, uint8_t **data) {
    z_ipc_msg_reader_t *reader = Z_IPC_MSG_READER(self);
    z_ipc_msgbuf_node_t *node = reader->node;
    unsigned int size;

    if (node == NULL)
        return(0);

    /* still some bytes left */
    size = z_min(reader->available, __msgbuf_node_used(node));
    if (reader->offset < size) {
        unsigned int n = size - reader->offset;
        *data = node->data + reader->offset;
        reader->offset = size;
        reader->available -= n;
        return(n);
    }

    if ((reader->node = node->next) == NULL)
        return(0);

    node = reader->node;
    size = z_min(reader->available, __msgbuf_node_used(node));
    reader->offset = size;
    reader->available -= size;
    *data = node->data;
    return(size);
}

static void __ipc_msg_reader_backup (void *self, size_t count) {
    z_ipc_msg_reader_t *reader = Z_IPC_MSG_READER(self);
    reader->available += count;
    reader->offset -= count;
}

/* ===========================================================================
 *  Slice vtables
 */
static const z_vtable_type_t __ipc_msg_type = {
    .name = "IPC-Message",
    .size = sizeof(z_ipc_msg_t),
    .ctor = __ipc_msg_ctor,
    .dtor = __ipc_msg_dtor,
};

static const z_vtable_reader_t __ipc_msg_reader = {
    .open       = __ipc_msg_reader_open,
    .close      = __ipc_msg_reader_close,
    .next       = __ipc_msg_reader_next,
    .backup     = __ipc_msg_reader_backup,
    .available  = NULL,
};

static const z_ipc_msg_interfaces_t __ipc_msg_interfaces = {
    .type   = &__ipc_msg_type,
    .reader = &__ipc_msg_reader,
};

/* ===========================================================================
 *  PUBLIC Slice constructor/destructor
 */
z_ipc_msg_t *z_ipc_msg_alloc (z_ipc_msg_t *self,
                              z_ipc_msgbuf_t *msgbuf,
                              uint32_t offset,
                              uint64_t length)
{
    return(z_object_alloc(self, &__ipc_msg_interfaces, msgbuf->memory,
                          msgbuf, offset, length));
}

void z_ipc_msg_add_node (z_ipc_msg_t *self, z_ipc_msgbuf_node_t *node) {
    z_atomic_inc(&(node->refs));
    self->blocks++;
}

void z_ipc_msg_free (z_ipc_msg_t *self) {
    z_object_free(self);
}

