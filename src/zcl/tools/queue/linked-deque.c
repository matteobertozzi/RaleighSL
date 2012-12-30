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

#include <zcl/linked-deque.h>

/* ===========================================================================
 *  PRIVATE Linked-Deque data structures
 */
struct z_linked_deque_node {
    z_linked_deque_node_t *next;
    z_linked_deque_node_t *prev;
    void *data;
};

/* ===========================================================================
 *  PUBLIC Linked-Deque methods
 */
int z_linked_deque_push_front (z_linked_deque_t *deque, void *data) {
    z_linked_deque_node_t *node;
    z_linked_deque_node_t *head;

    if ((node = z_object_struct_alloc(deque, z_linked_deque_node_t)) == NULL)
        return(1);

    node->data = data;
    node->next = head = deque->head;
    node->prev = NULL;

    if (head == NULL)
        deque->tail = node;
    deque->head = node;

    return(0);
}

int z_linked_deque_push_back (z_linked_deque_t *deque, void *data) {
    z_linked_deque_node_t *node;
    z_linked_deque_node_t *tail;

    if ((node = z_object_struct_alloc(deque, z_linked_deque_node_t)) == NULL)
        return(1);

    node->data = data;
    node->next = NULL;
    node->prev = tail = deque->tail;

    if (tail != NULL)
        tail->next = node;
    else
        deque->head = node;
    deque->tail = node;

    return(0);
}

void *z_linked_deque_pop_front (z_linked_deque_t *deque) {
    z_linked_deque_node_t *node;
    z_linked_deque_node_t *next;
    void *data;

    if ((node = deque->head) == NULL)
        return(NULL);

    if ((deque->head = next = node->next) != NULL)
        next->prev = NULL;
    else
        deque->tail = NULL;

    data = node->data;
    z_object_struct_free(deque, z_linked_deque_node_t, node);

    return(data);
}

void *z_linked_deque_pop_back (z_linked_deque_t *deque) {
    z_linked_deque_node_t *node;
    z_linked_deque_node_t *prev;
    void *data;

    if ((node = deque->tail) == NULL)
        return(NULL);

    if ((deque->tail = prev = node->prev) != NULL)
        prev->next = NULL;
    else
        deque->head = NULL;

    data = node->data;
    z_object_struct_free(deque, z_linked_deque_node_t, node);

    return(data);
}

void *z_linked_deque_peek_front (z_linked_deque_t *deque) {
    z_linked_deque_node_t *node;

    if ((node = deque->head) != NULL)
        return(node->data);

    return(NULL);
}

void *z_linked_deque_peek_back (z_linked_deque_t *deque) {
    z_linked_deque_node_t *node;

    if ((node = deque->tail) != NULL)
        return(node->data);

    return(NULL);
}

void z_linked_deque_clear (z_linked_deque_t *self) {
    z_linked_deque_node_t *next;
    z_linked_deque_node_t *p;

    for (p = self->head; p != NULL; p = next) {
        next = p->next;
        if (self->data_free != NULL)
            self->data_free(self->user_data, p->data);
        z_object_struct_free(self, z_linked_deque_node_t, p);
    }

    self->head = NULL;
    self->tail = NULL;
}

int z_linked_deque_is_empty (const z_linked_deque_t *self) {
    return(self->head == NULL);
}

/* ===========================================================================
 *  INTERFACE Deque methods
 */
static int __linked_deque_push_back (void *self, void *element) {
    return(z_linked_deque_push_back(Z_LINKED_DEQUE(self), element));
}

static int __linked_deque_push_front (void *self, void *element) {
    return(z_linked_deque_push_front(Z_LINKED_DEQUE(self), element));
}

static void *__linked_deque_peek_back (void *self) {
    return(z_linked_deque_peek_back(Z_LINKED_DEQUE(self)));
}

static void *__linked_deque_peek_front (void *self) {
    return(z_linked_deque_peek_front(Z_LINKED_DEQUE(self)));
}

static void *__linked_deque_pop_back (void *self) {
    return(z_linked_deque_pop_back(Z_LINKED_DEQUE(self)));
}

static void *__linked_deque_pop_front (void *self) {
    return(z_linked_deque_pop_front(Z_LINKED_DEQUE(self)));
}

static void __linked_deque_clear (void *self) {
    z_linked_deque_clear(Z_LINKED_DEQUE(self));
}

static int __linked_deque_is_empty (const void *self) {
    return(z_linked_deque_is_empty(Z_CONST_LINKED_DEQUE(self)));
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __linked_deque_ctor (void *self, z_memory_t *memory, va_list args) {
    z_linked_deque_t *deque = Z_LINKED_DEQUE(self);

    deque->head = NULL;
    deque->tail = NULL;
    deque->data_free = va_arg(args, z_mem_free_t);
    deque->user_data = va_arg(args, void *);

    return(0);
}

static void __linked_deque_dtor (void *self) {
    __linked_deque_clear(self);
}

/* ===========================================================================
 *  Deque vtables
 */
static const z_vtable_type_t __linked_deque_type = {
    .name = "LinkedDeque",
    .size = sizeof(z_linked_deque_t),
    .ctor = __linked_deque_ctor,
    .dtor = __linked_deque_dtor,
};

static const z_vtable_deque_t __linked_deque = {
    .push_back  = __linked_deque_push_back,
    .push_front = __linked_deque_push_front,
    .peek_back  = __linked_deque_peek_back,
    .peek_front = __linked_deque_peek_front,
    .pop_back   = __linked_deque_pop_back,
    .pop_front  = __linked_deque_pop_front,
    .clear      = __linked_deque_clear,
    .is_empty   = __linked_deque_is_empty,
};

static const z_deque_interfaces_t __linked_deque_interfaces = {
    .type       = &__linked_deque_type,
    .deque      = &__linked_deque,
};

/* ===========================================================================
 *  PUBLIC Skip-List constructor/destructor
 */
z_linked_deque_t *z_linked_deque_alloc (z_linked_deque_t *self,
                                        z_memory_t *memory,
                                        z_mem_free_t data_free,
                                        void *user_data)
{
    return(z_object_alloc(self, &__linked_deque_interfaces, memory,
                          data_free, user_data));
}

void z_linked_deque_free (z_linked_deque_t *self) {
    z_object_free(self);
}

