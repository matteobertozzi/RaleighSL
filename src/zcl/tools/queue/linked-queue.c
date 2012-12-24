/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#include <zcl/linked-queue.h>

/* ===========================================================================
 *  PRIVATE Linked-Queue data structures
 */
struct z_linked_queue_node {
    z_linked_queue_node_t *next;
    void *data;
};

/* ===========================================================================
 *  PUBLIC Linked-Queue methods
 */
int z_linked_queue_push (z_linked_queue_t *self, void *data) {
    z_linked_queue_node_t *node;
    z_linked_queue_node_t *tail;

    if ((node = z_object_struct_alloc(self, z_linked_queue_node_t)) == NULL)
        return(1);

    node->data = data;
    node->next = NULL;

    if ((tail = self->tail) != NULL) {
        tail->next = node;
        self->tail = node;
    } else {
        self->head = node;
        self->tail = node;
    }

    return(0);
}

void *z_linked_queue_peek (z_linked_queue_t *self) {
    z_linked_queue_node_t *node;

    if ((node = self->head) != NULL)
        return(node->data);

    return(NULL);
}

void *z_linked_queue_pop (z_linked_queue_t *self) {
    z_linked_queue_node_t *node;
    void *data;

    if ((node = self->head) == NULL)
        return(NULL);

    if ((self->head = node->next) == NULL)
        self->tail = NULL;

    data = node->data;
    z_object_struct_free(self, z_linked_queue_node_t, node);

    return(data);
}

void z_linked_queue_clear (z_linked_queue_t *self) {
    z_linked_queue_node_t *next;
    z_linked_queue_node_t *p;

    for (p = self->head; p != NULL; p = next) {
        next = p->next;
        if (self->data_free != NULL)
            self->data_free(self->user_data, p->data);
        z_object_struct_free(self, z_linked_queue_node_t, p);
    }

    self->head = NULL;
    self->tail = NULL;
}

int z_linked_queue_is_empty (const z_linked_queue_t *self) {
    return(self->head == NULL);
}

/* ===========================================================================
 *  INTERFACE Queue methods
 */
static int __linked_queue_push (void *self, void *element) {
    return(z_linked_queue_push(Z_LINKED_QUEUE(self), element));
}

static void *__linked_queue_peek (void *self) {
    return(z_linked_queue_peek(Z_LINKED_QUEUE(self)));
}

static void *__linked_queue_pop (void *self) {
    return(z_linked_queue_pop(Z_LINKED_QUEUE(self)));
}

static void __linked_queue_clear (void *self) {
    z_linked_queue_clear(Z_LINKED_QUEUE(self));
}

static int __linked_queue_is_empty (const void *self) {
    return(z_linked_queue_is_empty(Z_CONST_LINKED_QUEUE(self)));
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __linked_queue_ctor (void *self, z_memory_t *memory, va_list args) {
    z_linked_queue_t *queue = Z_LINKED_QUEUE(self);

    queue->head = NULL;
    queue->tail = NULL;
    queue->data_free = va_arg(args, z_mem_free_t);
    queue->user_data = va_arg(args, void *);

    return(0);
}

static void __linked_queue_dtor (void *self) {
    __linked_queue_clear(self);
}

/* ===========================================================================
 *  Queue vtables
 */
static const z_vtable_type_t __linked_queue_type = {
    .name = "LinkedQueue",
    .size = sizeof(z_linked_queue_t),
    .ctor = __linked_queue_ctor,
    .dtor = __linked_queue_dtor,
};

static const z_vtable_queue_t __linked_queue = {
    .push       = __linked_queue_push,
    .peek       = __linked_queue_peek,
    .pop        = __linked_queue_pop,
    .clear      = __linked_queue_clear,
    .is_empty   = __linked_queue_is_empty,
};

static const z_queue_interfaces_t __linked_queue_interfaces = {
    .type       = &__linked_queue_type,
    .queue      = &__linked_queue,
};

/* ===========================================================================
 *  PUBLIC Skip-List constructor/destructor
 */
z_linked_queue_t *z_linked_queue_alloc (z_linked_queue_t *self,
                                        z_memory_t *memory,
                                        z_mem_free_t data_free,
                                        void *user_data)
{
    return(z_object_alloc(self, &__linked_queue_interfaces, memory, 
                          data_free, user_data));
}

void z_linked_queue_free (z_linked_queue_t *self) {
    z_object_free(self);
}

