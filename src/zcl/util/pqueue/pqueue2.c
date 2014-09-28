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

#include <string.h>

void z_pqueue2_init (z_pqueue2_t *self, uint8_t isize, void *buffer) {
  self->buffer = buffer;
  self->ihead  = 0;
  self->isize  = isize;
  self->count  = 0;
}

void *z_pqueue2_add (z_pqueue2_t *self, const void *item) {
  void *p;

  if (self->count++ > 0) {
    int cmp;
    p = self->buffer + (((self->ihead + 1) & 1) * self->isize);
    cmp = memcmp(item, self->buffer + self->ihead * self->isize, self->isize);
    self->ihead = (self->ihead + (cmp < 0)) & 1;
  } else {
    p = self->buffer + (self->ihead * self->isize);
  }

  return(p);
}

void *z_pqueue2_peek (z_pqueue2_t *self) {
  void *p = self->buffer + (self->ihead * self->isize);
  self->ihead = (self->ihead + 1) & 1;
  return(p);
}
