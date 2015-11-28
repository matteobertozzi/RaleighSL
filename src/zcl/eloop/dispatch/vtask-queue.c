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

#include <zcl/vtask.h>
#include <zcl/debug.h>

#define __QNODE_NEXT            (0)
#define __QNODE_PREV            (1)

#define __VTASK_QUEUE_DEBUG__   (0 && __Z_DEBUG__)

#if __VTASK_QUEUE_DEBUG__
static void __verify_queue (z_vtask_queue_t *self) {
  z_vtask_t *p;
  if (self->head != NULL) {
    z_vtask_t *tail = self->head->link.child[__QNODE_PREV];
    Z_ASSERT(tail->link.child[__QNODE_NEXT] == NULL, "Wrong head/tail ptr");
  }
  for (p = self->head; p != NULL; p = p->link.child[__QNODE_NEXT]) {
    z_vtask_t *next = p->link.child[__QNODE_NEXT];
    Z_ASSERT(p->attached, "%"PRIu64" Not attached", p->seqid);
    if (next == NULL) {
      Z_ASSERT(p == self->head->link.child[__QNODE_PREV], "Wrong head/tail ptr");
    }
  }
}
#endif /* __VTASK_QUEUE_DEBUG__ */

void z_vtask_queue_init (z_vtask_queue_t *self) {
  self->head = NULL;
}

void z_vtask_queue_push (z_vtask_queue_t *self, z_vtask_t *vtask) {
  if (self->head == NULL) {
    self->head = vtask;
    vtask->link.child[__QNODE_NEXT] = NULL;
    vtask->link.child[__QNODE_PREV] = vtask;
  } else {
    z_vtask_t *head = self->head;
    z_vtask_t *tail;

    tail = head->link.child[__QNODE_PREV];
    Z_ASSERT(tail != NULL, "Missing Prev Link");

    tail->link.child[__QNODE_NEXT] = vtask;
    head->link.child[__QNODE_PREV] = vtask;
    vtask->link.child[__QNODE_NEXT] = NULL;
    vtask->link.child[__QNODE_PREV] = tail;
  }
  vtask->attached = 1;

#if __VTASK_QUEUE_DEBUG__
  __verify_queue(self);
#endif
}

z_vtask_t *z_vtask_queue_pop (z_vtask_queue_t *self) {
  z_vtask_t *vtask = self->head;
  z_vtask_t *next;

  Z_ASSERT(self->head != NULL, "Unexpected null queue");
  next = vtask->link.child[__QNODE_NEXT];
  if ((self->head = next) != NULL) {
    next->link.child[__QNODE_PREV] = vtask->link.child[__QNODE_PREV];
  }

  vtask->link.child[__QNODE_NEXT] = NULL;
  vtask->link.child[__QNODE_PREV] = NULL;
  vtask->attached = 0;

#if __VTASK_QUEUE_DEBUG__
  __verify_queue(self);
#endif
  return(vtask);
}

void z_vtask_queue_remove (z_vtask_queue_t *self, z_vtask_t *vtask) {
  z_vtask_t *next = vtask->link.child[__QNODE_NEXT];
  z_vtask_t *prev = vtask->link.child[__QNODE_PREV];

  Z_ASSERT(self->head != NULL, "Unexpected null queue");
  Z_ASSERT(vtask->attached, "Not attached");

  if (self->head == vtask) {
    self->head = next;
    if (next)
      next->link.child[__QNODE_PREV] = prev;
  } else if (next == NULL) {
    prev->link.child[__QNODE_NEXT] = NULL;
    self->head->link.child[__QNODE_PREV] = prev;
  } else {
    prev->link.child[__QNODE_NEXT] = next;
    next->link.child[__QNODE_PREV] = prev;
  }

  vtask->link.child[__QNODE_NEXT] = NULL;
  vtask->link.child[__QNODE_PREV] = NULL;
  vtask->attached = 0;

#if __VTASK_QUEUE_DEBUG__
  __verify_queue(self);
#endif
}

void z_vtask_queue_move_to_tail (z_vtask_queue_t *self) {
  z_vtask_t *vtask = self->head;
  z_vtask_t *next;

  Z_ASSERT(self->head != NULL, "Unexpected null queue");
  next = vtask->link.child[__QNODE_NEXT];
  if (next != NULL) {
    z_vtask_t *prev = vtask->link.child[__QNODE_PREV];

    self->head = next;
    next->link.child[__QNODE_PREV] = vtask;
    prev->link.child[__QNODE_NEXT] = vtask;
    vtask->link.child[__QNODE_NEXT] = NULL;
  }

#if __VTASK_QUEUE_DEBUG__
  __verify_queue(self);
#endif
}

void z_vtask_queue_cancel (z_vtask_queue_t *self) {
  z_vtask_t *p;
  for (p = self->head; p != NULL; p = p->link.child[__QNODE_NEXT]) {
    p->cancel = 1;
  }
}

z_vtask_t *z_vtask_queue_next (z_vtask_t *vtask) {
  return(vtask->link.child[__QNODE_NEXT]);
}

void z_vtask_queue_dump (z_vtask_queue_t *self, FILE *stream) {
  if (self->head != NULL) {
    z_vtask_t *p;
    if (self->head != NULL) {
      z_vtask_t *tail = self->head->link.child[__QNODE_PREV];
      Z_ASSERT(tail->link.child[__QNODE_NEXT] == NULL, "Wrong head/tail ptr");
    }

    fprintf(stream, "Task-Queue %p: ", self);
    for (p = self->head; p != NULL; p = p->link.child[__QNODE_NEXT]) {
      z_vtask_t *prev = p->link.child[__QNODE_PREV];
      z_vtask_t *next = p->link.child[__QNODE_NEXT];
      Z_ASSERT(p->attached, "%"PRIu64" Not attached", p->seqid);
      fprintf(stream, "[%"PRIu64"<-%"PRIu64"->%"PRIu64"] -> ",
              (prev ? prev->seqid : 0), p->seqid, (next ? next->seqid : 0));
      if (next == NULL) {
        Z_ASSERT(p == self->head->link.child[__QNODE_PREV], "Wrong head/tail ptr");
      }
    }
    fprintf(stream, "X\n");
  } else {
    fprintf(stream, "Task-Queue %p: NULL\n", self);
  }
}
