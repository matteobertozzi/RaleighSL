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

#include <zcl/pqueue.h>
#include <zcl/search.h>

#include <string.h>

int z_pqueue_open (z_pqueue_t *self,
                   uint16_t isize,
                   void *objects,
                   uint32_t length)
{
  self->head = 0;
  self->tail = 0;
  self->isize = isize;
  self->alloc = (objects == NULL);
  self->olength = length;
  if (objects != NULL) {
    self->objects = objects;
  } else {
    self->objects = (uint8_t *) malloc(length * isize);
  }
  return(self->objects == NULL);
}

void z_pqueue_close (z_pqueue_t *self) {
  if (self->alloc) {
    free(self->objects);
  }
}

void z_pqueue_add (z_pqueue_t *self, void *elem, z_compare_t key_cmp, void *udata) {
  memcpy(z_pqueue_add_slot(self, elem, key_cmp, udata), elem, self->isize);
}


#define __pqueue_upper_bound(self, key, key_cmp, udata)               \
  z_upper_bound((self)->objects + (self)->head, (self)->tail - 1,     \
                (self)->isize, key, key_cmp, udata)

uint8_t *z_pqueue_add_slot (z_pqueue_t *self, void *elem, z_compare_t key_cmp, void *udata) {
  uint8_t *ptr;

  if (self->tail == self->olength) {
    if (self->head == 0) {
      /* no space left */
      return(NULL);
    }

    /* shift down |-----AAAAAAA| */
    self->tail -= self->head;
    memmove(self->objects, z_pqueue_first(self), self->tail * self->isize);
    self->head = 0;
  }

  if (self->tail == self->head || key_cmp(udata, z_pqueue_last(self), elem) <= 0) {
    // Append
    ptr = self->objects + (self->tail++ * self->isize);
  } else if (self->head > 0 && key_cmp(udata, z_pqueue_first(self), elem) > 0) {
    // Prepend
    ptr = self->objects + (--(self->head) * self->isize);
  } else {
    // Insert in the middle
    int index = __pqueue_upper_bound(self, elem, key_cmp, udata);
    ptr = self->objects + (index * self->isize);
    memmove(ptr + self->isize, ptr, (self->tail - index) * self->isize);
    self->tail++;
  }
  return(ptr);
}

uint8_t *z_pqueue_poll (z_pqueue_t *self) {
  uint8_t *elem = z_pqueue_first(self);
  self->head = (self->head + 1) % self->olength;
  if (self->head == 0)
    self->tail = 0;
  return(elem);
}
