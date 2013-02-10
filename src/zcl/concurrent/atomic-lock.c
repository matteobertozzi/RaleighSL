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

#include <zcl/atomic.h>
#ifndef Z_CPU_HAS_LOCK_FREE_SUPPORT

#include <zcl/threading.h>

Z_TYPEDEF_STRUCT(z_atomic_node)

struct z_atomic_node {
    z_atomic_node_t *next;
    void *data;
};

struct z_atomic_stack {
    z_memory_t *memory;
    z_spinlock_t lock;
    z_atomic_node_t *head;
};

struct z_atomic_queue {
    z_memory_t *memory;
    z_spinlock_t hlock;
    z_atomic_node_t *head;
    z_spinlock_t tlock;
    z_atomic_node_t *tail;
};

/* ============================================================================
 *  Atomic Node Helpers
 */
static z_atomic_node_t *__atomic_node_alloc (z_memory_t *memory, void *data) {
    z_atomic_node_t *node;

    if ((node = z_memory_struct_alloc(memory, z_atomic_node_t)) != NULL) {
      node->data = data;
    }

    return(node);
}

static void __atomic_node_free (z_memory_t *memory, z_atomic_node_t *node) {
  z_memory_struct_free(memory, z_atomic_node_t, node);
}

/* ============================================================================
 *  Atomic Stack (with locks)
 */
z_atomic_stack_t *z_atomic_stack_alloc (z_memory_t *memory) {
    z_atomic_stack_t *stack;

    if ((stack = z_memory_struct_alloc(memory, z_atomic_stack_t)) == NULL)
        return(NULL);

    if (z_spin_alloc(&(stack->lock))) {
        z_memory_struct_free(memory, z_atomic_stack_t, stack);
        return(NULL);
    }

    stack->memory = memory;
    stack->head = NULL;
    return(stack);
}

int z_atomic_stack_free (z_atomic_stack_t *stack) {
    if (stack->head != NULL)
        return(1);

    z_memory_struct_free(stack->memory, z_atomic_stack_t, stack);
    return(0);
}

int z_atomic_stack_push (z_atomic_stack_t *stack, void *data) {
    z_atomic_node_t *node;

    if (Z_UNLIKELY((node = __atomic_node_alloc(stack->memory, data)) == NULL))
      return(1);

    z_spin_lock(&(stack->lock));
    node->next = stack->head;
    stack->head = node;
    z_spin_unlock(&(stack->lock));
    return(0);
}

void *z_atomic_stack_pop (z_atomic_stack_t *stack) {
    z_atomic_node_t *node;
    void *data;

    z_spin_lock(&(stack->lock));
    node = stack->head;
    stack->head = node->next;
    z_spin_unlock(&(stack->lock));

    data = node->data;
    __atomic_node_free(stack->memory, node);
    return(data);
}

/* ============================================================================
 *  Atomic Queue (with locks)
 */
z_atomic_queue_t *z_atomic_queue_alloc (z_memory_t *memory) {
    z_atomic_queue_t *queue;
    z_atomic_node_t *dummy;

    if ((queue = z_memory_struct_alloc(memory, z_atomic_queue_t)) == NULL)
        return(NULL);

    if ((dummy = __atomic_node_alloc(memory, NULL)) == NULL) {
        z_memory_struct_free(memory, z_atomic_queue_t, queue);
        return(NULL);
    }

    if (z_spin_alloc(&(queue->hlock))) {
        __atomic_node_free(memory, dummy);
        z_memory_struct_free(memory, z_atomic_queue_t, queue);
        return(NULL);
    }

    if (z_spin_alloc(&(queue->tlock))) {
        z_spin_free(&(queue->hlock));
        __atomic_node_free(memory, dummy);
        z_memory_struct_free(memory, z_atomic_queue_t, queue);
        return(NULL);
    }


    queue->memory = memory;
    queue->head = dummy;
    queue->tail = dummy;
    return(queue);
}

int z_atomic_queue_free (z_atomic_queue_t *queue) {
    if (queue->head != queue->tail)
        return(1);

    z_spin_free(&(queue->hlock));
    z_spin_free(&(queue->tlock));
    __atomic_node_free(queue->memory, queue->head);
    z_memory_struct_free(queue->memory, z_atomic_queue_t, queue);
    return(0);
}

int z_atomic_queue_push (z_atomic_queue_t *queue, void *data) {
    z_atomic_node_t *node;

    if (Z_UNLIKELY((node = __atomic_node_alloc(queue->memory, data)) == NULL))
      return(1);

    node->next = NULL;
    z_spin_lock(&(queue->tlock));
    queue->tail->next = node;
    queue->tail = node;
    z_spin_unlock(&(queue->tlock));
    return(0);
}

void *z_atomic_queue_pop (z_atomic_queue_t *queue) {
    z_atomic_node_t *new_head;
    z_atomic_node_t *node;
    void *data;

    z_spin_lock(&(queue->hlock));
    node = queue->head;
    if ((new_head = node->next) == NULL) {
        z_spin_unlock(&(queue->hlock));
        return(NULL);
    }

    data = new_head->data;
    queue->head = new_head;
    z_spin_unlock(&(queue->hlock));

    __atomic_node_free(queue->memory, node);
    return(data);
}

#endif /* !Z_CPU_HAS_LOCK_FREE_SUPPORT */
