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

#include <zcl/freelist.h>

/* ===========================================================================
 *  PRIVATE Free-List data structures
 */
struct z_free_list_node {
    z_free_list_node_t *next;
    size_t size;
};

/* ===========================================================================
 *  PUBLIC Free-List methods
 */
void z_free_list_add (z_free_list_t *self, void *memory) {
    z_free_list_node_t *node = Z_FREE_LIST_NODE(memory);
    node->next = self->head;
    self->head = node;
}

void z_free_list_add_sized (z_free_list_t *self, void *memory, size_t size) {
    z_free_list_node_t *node = Z_FREE_LIST_NODE(memory);
    node->next = self->head;
    node->size = size;
    self->head = node;
}

void *z_free_list_pop (z_free_list_t *self) {
    z_free_list_node_t *node = self->head;
    self->head = node->next;
    return(node);
}

void *z_free_list_pop_fit (z_free_list_t *self, size_t required, size_t *size) {
    z_free_list_node_t *prev = self->head;
    z_free_list_node_t *node = self->head;

    while (node != NULL) {
        if (node->size >= required) {
            prev->next = node->next;
            *size = node->size;
            return(node);
        }

        prev = node;
        node = node->next;
    }

    return(NULL);
}

void *z_free_list_pop_best (z_free_list_t *self, size_t required, size_t *size)
{
    z_free_list_node_t *prev = self->head;
    z_free_list_node_t *node = self->head;
    z_free_list_node_t *best_prev = NULL;
    z_free_list_node_t *best = NULL;

    while (node != NULL) {
        if (node->size == required) {
            prev->next = node->next;
            *size = node->size;
            return(node);
        }

        if (node->size > required && best == NULL) {
            best_prev = node;
            best = node;
        }

        prev = node;
        node = node->next;
    }

    if (best != NULL) {
        best_prev->next = best->next;
        *size = best->size;
        return(best);
    }

    return(NULL);
}
