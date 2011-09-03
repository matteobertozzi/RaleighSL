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

#include <zcl/deque.h>

z_deque_t *z_deque_alloc (z_deque_t *deque,
                          z_memory_t *memory,
                          z_mem_free_t data_free,
                          void *user_data)
{
    if ((deque = z_object_alloc(memory, deque, z_deque_t)) == NULL)
        return(NULL);

    deque->head = NULL;
    deque->tail = NULL;
    deque->data_free = data_free;
    deque->user_data = user_data;

    return(deque);
}

void z_deque_free (z_deque_t *deque) {
    z_deque_clear(deque);
    z_object_free(deque);
}

void z_deque_clear (z_deque_t *deque) {
    z_deque_node_t *next;
    z_deque_node_t *p;

    for (p = deque->head; p != NULL; p = next) {
        next = p->next;
        if (deque->data_free != NULL)
            deque->data_free(deque->user_data, p->data);
        z_object_struct_free(deque, z_deque_node_t, p);
    }

    deque->head = NULL;
    deque->tail = NULL;
}

int z_deque_push_front (z_deque_t *deque, void *data) {
    z_deque_node_t *node;
    z_deque_node_t *head;

    if ((node = z_object_struct_alloc(deque, z_deque_node_t)) == NULL)
        return(1);

    node->data = data;
    node->next = head = deque->head;
    node->prev = NULL;

    if (head == NULL)
        deque->tail = node;
    deque->head = node;

    return(0);
}

int z_deque_push_back (z_deque_t *deque, void *data) {
    z_deque_node_t *node;
    z_deque_node_t *tail;

    if ((node = z_object_struct_alloc(deque, z_deque_node_t)) == NULL)
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

void *z_deque_pop_front (z_deque_t *deque) {
    z_deque_node_t *node;
    z_deque_node_t *next;
    void *data;

    if ((node = deque->head) == NULL)
        return(NULL);

    if ((deque->head = next = node->next) != NULL)
        next->prev = NULL;
    else
        deque->tail = NULL;

    data = node->data;
    z_object_struct_free(deque, z_deque_node_t, node);

    return(data);
}

void *z_deque_pop_back (z_deque_t *deque) {
    z_deque_node_t *node;
    z_deque_node_t *prev;
    void *data;

    if ((node = deque->tail) == NULL)
        return(NULL);

    if ((deque->tail = prev = node->prev) != NULL)
        prev->next = NULL;
    else
        deque->head = NULL;

    data = node->data;
    z_object_struct_free(deque, z_deque_node_t, node);

    return(data);
}

void *z_deque_peek_front (z_deque_t *deque) {
    z_deque_node_t *node;

    if ((node = deque->head) != NULL)
        return(node->data);

    return(NULL);
}

void *z_deque_peek_back (z_deque_t *deque) {
    z_deque_node_t *node;

    if ((node = deque->tail) != NULL)
        return(node->data);

    return(NULL);
}

int z_deque_is_empty (z_deque_t *deque) {
    return(deque->head == NULL);
}

