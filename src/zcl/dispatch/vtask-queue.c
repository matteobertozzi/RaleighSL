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

void z_vtask_queue_init (z_vtask_queue_t *self) {
  self->head = NULL;
}

void z_vtask_queue_push (z_vtask_queue_t *self, z_vtask_t *vtask) {
  if (self->head == NULL) {
    vtask->link.child[0]  = NULL;
    vtask->link.child[1] = vtask;
    self->head = vtask;
  } else {
    z_vtask_t *head = self->head;
    z_vtask_t *tail = head->link.child[1];

    tail->link.child[0]  = vtask;
    head->link.child[1]  = vtask;
    vtask->link.child[0] = NULL;
    vtask->link.child[1] = NULL;
  }
}

z_vtask_t *z_vtask_queue_pop (z_vtask_queue_t *self) {
  z_vtask_t *vtask = self->head;
  z_vtask_t *next;

  next = vtask->link.child[0];
  if ((self->head = next) != NULL)
    next->link.child[1] = vtask->link.child[1];

  vtask->link.child[0] = NULL;
  vtask->link.child[1] = NULL;
  return(vtask);
}
