/*
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

#include <zcl/locking.h>
#include <zcl/system.h>
#include <zcl/memory.h>
#include <zcl/global.h>

struct atomic_node {
  struct atomic_node *next;
  void *data;
};

struct atomic_stack {
  z_spinlock_t lock;
  struct atomic_node *head;
};

struct atomic_queue {
  z_spinlock_t hlock;
  struct atomic_node *head;
  uint8_t __pad0[Z_CACHELINE_PAD(sizeof(void *) + sizeof(z_spinlock_t))];

  z_spinlock_t tlock;
  struct atomic_node *tail;
  uint8_t __pad1[Z_CACHELINE_PAD(sizeof(void *) + sizeof(z_spinlock_t))];
};

/* ============================================================================
 *  Atomic Node Helpers
 */
static struct atomic_node *__atomic_node_alloc (void *data) {
  struct atomic_node *node;

  node = z_memory_struct_alloc(z_global_memory(), struct atomic_node);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  node->data = data;
  return(node);
}

static void __atomic_node_free (struct atomic_node *node) {
  z_memory_struct_free(z_global_memory(), struct atomic_node, node);
}

/* ============================================================================
 *  Atomic Stack (with locks)
 */
int z_atomic_stack_alloc (z_atomic_stack_t *self) {
  struct atomic_stack *stack = (struct atomic_stack *)self;

  if (z_spin_alloc(&(stack->lock))) {
    return(1);
  }

  stack->head = NULL;
  return(0);
}

int z_atomic_stack_free (z_atomic_stack_t *self) {
  struct atomic_stack *stack = (struct atomic_stack *)self;

  if (stack->head != NULL)
    return(1);

  z_spin_free(&(stack->lock));
  return(0);
}

int z_atomic_stack_push (z_atomic_stack_t *self, void *data) {
  struct atomic_stack *stack = (struct atomic_stack *)self;
  struct atomic_node *node;

  node = __atomic_node_alloc(data);
  if (Z_MALLOC_IS_NULL(node))
    return(1);

  z_spin_lock(&(stack->lock));
  node->next = stack->head;
  stack->head = node;
  z_spin_unlock(&(stack->lock));
  return(0);
}

void *z_atomic_stack_pop (z_atomic_stack_t *self) {
  struct atomic_stack *stack = (struct atomic_stack *)self;
  struct atomic_node *node;
  void *data;

  z_spin_lock(&(stack->lock));
  node = stack->head;
  stack->head = node->next;
  z_spin_unlock(&(stack->lock));

  data = node->data;
  __atomic_node_free(node);
  return(data);
}

/* ============================================================================
 *  Atomic Queue (with locks)
 */
int z_atomic_queue_alloc (z_atomic_queue_t *self) {
  struct atomic_queue *queue = (struct atomic_queue *)self;
  struct atomic_node *dummy;

  if ((dummy = __atomic_node_alloc(NULL)) == NULL) {
    return(1);
  }

  if (z_spin_alloc(&(queue->hlock))) {
    __atomic_node_free(dummy);
    return(2);
  }

  if (z_spin_alloc(&(queue->tlock))) {
    z_spin_free(&(queue->hlock));
    __atomic_node_free(dummy);
    return(3);
  }

  queue->head = dummy;
  queue->tail = dummy;
  return(0);
}

int z_atomic_queue_free (z_atomic_queue_t *self) {
  struct atomic_queue *queue = (struct atomic_queue *)self;

  if (queue->head != queue->tail)
    return(1);

  z_spin_free(&(queue->hlock));
  z_spin_free(&(queue->tlock));
  __atomic_node_free(queue->head);
  return(0);
}

int z_atomic_queue_push (z_atomic_queue_t *self, void *data) {
  struct atomic_queue *queue = (struct atomic_queue *)self;
  struct atomic_node *node;

  node = __atomic_node_alloc(data);
  if (Z_MALLOC_IS_NULL(node))
    return(1);

  node->next = NULL;
  z_lock(&(queue->tlock), z_spin, {
    queue->tail->next = node;
    queue->tail = node;
  });

  return(0);
}

void *z_atomic_queue_pop (z_atomic_queue_t *self) {
  struct atomic_queue *queue = (struct atomic_queue *)self;
  struct atomic_node *new_head;
  struct atomic_node *node;
  void *data = NULL;

  z_lock(&(queue->hlock), z_spin, {
    node = queue->head;
    if ((new_head = node->next) != NULL) {
      data = new_head->data;
      queue->head = new_head;
    }
  });

  if (data != NULL)
    __atomic_node_free(node);
  return(data);
}

#endif /* !Z_CPU_HAS_LOCK_FREE_SUPPORT */
