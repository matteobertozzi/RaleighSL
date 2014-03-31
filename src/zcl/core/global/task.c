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

#include <zcl/global.h>
#include <zcl/task.h>
#include <zcl/time.h>

/* ===========================================================================
 *  PRIVATE Task Tree
 */
static int __task_tree_seqid_compare (void *udata, const void *a, const void *b) {
  const z_task_t *ea = z_container_of(a, const z_task_t, sys_node);
  const z_task_t *eb = z_container_of(b, const z_task_t, sys_node);
  return(z_cmp(z_task_seqid(ea), z_task_seqid(eb)));
}

static int __task_tree_vtime_compare (void *udata, const void *a, const void *b) {
  const z_task_t *ea = z_container_of(a, const z_task_t, sys_node);
  const z_task_t *eb = z_container_of(b, const z_task_t, sys_node);
  int cmp = z_cmp(z_task_vtime(ea), z_task_vtime(eb));
  return(cmp != 0 ? cmp : z_cmp(z_task_seqid(ea), z_task_seqid(eb)));
}

const z_tree_info_t z_task_seqid_tree_info = {
  .plug         = &z_tree_avl,
  .node_compare = __task_tree_seqid_compare,
  .key_compare  = __task_tree_seqid_compare,
  .node_free    = NULL,
};

const z_tree_info_t z_task_vtime_tree_info = {
  .plug         = &z_tree_avl,
  .node_compare = __task_tree_vtime_compare,
  .key_compare  = __task_tree_vtime_compare,
  .node_free    = NULL,
};

/* ===========================================================================
 *  PUBLIC Task queue
 */
void z_task_queue_open (z_task_queue_t *self) {
  self->head = NULL;
}

void z_task_queue_close (z_task_queue_t *self) {
}

void z_task_queue_push (z_task_queue_t *self, z_task_t *task) {
  if (task != NULL) {
    z_tree_node_t *node = &(task->sys_node);
    if (self->head != NULL) {
      z_tree_node_t *head = &(self->head->sys_node);
      z_tree_node_t *tail = head->child[1];

      tail->child[0] = node;
      head->child[1] = node;
      node->child[0] = NULL;
      node->child[1] = NULL;
    } else {
      node->child[0] = NULL;
      node->child[1] = node;
      self->head = task;
    }
  }
}

z_task_t *z_task_queue_pop (z_task_queue_t *self) {
  if (self->head != NULL) {
    z_task_t *task = self->head;
    z_tree_node_t *head = &(self->head->sys_node);
    z_tree_node_t *next = head->child[0];
    z_tree_node_t *tail = head->child[1];

    if ((self->head = z_container_of(next, z_task_t, sys_node)) != NULL)
      next->child[1] = tail;

    task->sys_node.child[0] = NULL;
    task->sys_node.child[1] = NULL;
    return(task);
  }
  return(NULL);
}

z_task_t *z_task_queue_drain (z_task_queue_t *self) {
  z_task_t *task = self->head;
  self->head = NULL;
  return(task);
}

/* ===========================================================================
 *  PUBLIC Task Tree
 */
void z_task_tree_open (z_task_tree_t *self) {
  self->root = NULL;
}

void z_task_tree_close (z_task_tree_t *self) {
}

void z_task_tree_push (z_task_tree_t *self, z_task_t *task) {
  while (task != NULL) {
    z_task_t *next = Z_TASK(task->sys_node.child[0]);
    z_tree_node_attach(&z_task_seqid_tree_info, &(self->root), &(task->sys_node), NULL);
    task = next;
  }
}

z_task_t *z_task_tree_pop (z_task_tree_t *self) {
  if (self->root != NULL) {
    return(Z_TASK(z_tree_node_detach_min(&z_task_seqid_tree_info, &(self->root))));
  }
  return(NULL);
}

