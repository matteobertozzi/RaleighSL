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
    unsigned int node_size;
    uint64_t buf_required;
    uint64_t flen;
    uint16_t fid;
    int elen;

    /* Nothing in here, sorry */
    if ((node = msgbuf->head) == NULL)
        return(NULL);

    if ((node_size = (__msgbuf_node_used(node) - msgbuf->offset)) == 0)
        return(NULL);

    Z_BUG_ON(msgbuf->length == 0, "Node with data on a empty buffer!");

    /* Decode the msg header */
    elen = z_decode_field(node->data + msgbuf->offset, node_size, &fid, &flen);
    if (elen < 0 && node->next != NULL && node_size < 16) {
        unsigned char buffer[16];
        unsigned int next_size;
        z_memcpy(buffer, node->data + msgbuf->offset, node_size);
        next_size = z_min(16, msgbuf->length) - node_size;
        z_memcpy(buffer + node_size, node->next->data, next_size);
        elen = z_decode_field(buffer, node_size + next_size, &fid, &flen);
    }

    /* No enough data to decode the field */
    buf_required = (elen + flen);
    if (elen < 0 || buf_required > msgbuf->length)
        return(NULL);

    /* Skip tag nodes */
    while (elen >= node_size) {
        z_ipc_msgbuf_node_t *head = node;

        /* first node can be skipped, no data here */
        node = node->next;
        __msgbuf_node_free(msgbuf, head);
        msgbuf->head = node;
        msgbuf->offset = 0;

        elen -= node_size;
        node_size = __msgbuf_node_used(node);
    }

    /* Update msgbuf size */
    node_size -= elen;
    msgbuf->length -= buf_required;
    msgbuf->offset += elen;

    /* Initialize the message references */
    if ((msg = z_ipc_msg_alloc(msg, msgbuf->memory)) == NULL)
        return(NULL);

    msg->msgbuf = msgbuf;
    msg->head = node;
    msg->offset = msgbuf->offset;
    msg->length = flen;
    msg->blocks = 0;
    msg->id = fid;

    /* Keep a references to the chunks that compose the message */
    while (1) {
        z_atomic_inc(&(node->refs));
        msg->blocks++;

        if (flen <= node_size) {
            msgbuf->offset += flen;
            break;
        }

        /* switch head */
        node = node->next;
        __msgbuf_node_free(msgbuf, msgbuf->head);
        msgbuf->head = node;
        msgbuf->offset = 0;

        flen -= node_size;
        node_size = __msgbuf_node_used(node);
    }

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
    /* TODO: use flag16 to store id? */
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
    .name = "Slice",
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
z_ipc_msg_t *z_ipc_msg_alloc (z_ipc_msg_t *self, z_memory_t *memory) {
    return(z_object_alloc(self, &__ipc_msg_interfaces, memory));
}

void z_ipc_msg_free (z_ipc_msg_t *self) {
    z_object_free(self);
}

