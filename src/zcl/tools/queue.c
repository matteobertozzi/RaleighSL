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

#include <zcl/queue.h>

z_queue_t *z_queue_alloc (z_queue_t *queue,
                          z_memory_t *memory,
                          z_mem_free_t data_free,
                          void *user_data)
{
    if ((queue = z_object_alloc(memory, queue, z_queue_t)) == NULL)
        return(NULL);

    queue->head = NULL;
    queue->tail = NULL;
    queue->data_free = data_free;
    queue->user_data = user_data;

    return(queue);
}

void z_queue_free (z_queue_t *queue) {
    z_queue_clear(queue);
    z_object_free(queue);
}

void z_queue_clear (z_queue_t *queue) {
    z_queue_node_t *p;

    for (p = queue->head; p != NULL; p = p->next) {
        if (queue->data_free != NULL)
            queue->data_free(queue->user_data, p->data);
        z_object_struct_free(queue, z_queue_node_t, p);
    }

    queue->head = NULL;
    queue->tail = NULL;
}

int z_queue_push (z_queue_t *queue, void *data) {
    z_queue_node_t *node;
    z_queue_node_t *tail;

    if ((node = z_object_struct_alloc(queue, z_queue_node_t)) == NULL)
        return(1);

    node->data = data;
    node->next = NULL;

    if ((tail = queue->tail) != NULL) {
        tail->next = node;
        queue->tail = node;
    } else {
        queue->head = node;
        queue->tail = node;
    }

    return(0);
}

void *z_queue_pop (z_queue_t *queue) {
    z_queue_node_t *node;
    void *data;

    if ((node = queue->head) == NULL)
        return(NULL);

    if ((queue->head = node->next) == NULL)
        queue->tail = NULL;

    data = node->data;
    z_object_struct_free(queue, z_queue_node_t, node);

    return(data);
}

void *z_queue_peek (z_queue_t *queue) {
    z_queue_node_t *node;

    if ((node = queue->head) != NULL)
        return(node->data);

    return(NULL);
}

int z_queue_is_empty (z_queue_t *queue) {
    return(queue->head == NULL);
}

