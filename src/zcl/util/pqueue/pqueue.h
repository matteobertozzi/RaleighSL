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

#ifndef _Z_PQUEUE_H_
#define _Z_PQUEUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/debug.h>

typedef struct z_pqueue {
  uint32_t head;
  uint32_t tail;
  uint16_t isize;
  uint16_t alloc;
  uint32_t olength;
  uint8_t *objects;
} z_pqueue_t;

typedef struct z_pqueue2 {
  uint8_t *buffer;
  uint8_t ihead;
  uint8_t isize;
  uint8_t count;
  uint8_t _pad;
} z_pqueue2_t;

#define z_pqueue_is_empty(self)         ((self)->head == (self)->tail)
#define z_pqueue_is_not_empty(self)     ((self)->head != (self)->tail)
#define z_pqueue_size(self)             ((self)->tail - (self)->head)

#define z_pqueue_first(self)                                    \
  ((self)->objects + ((self)->head) * (self)->isize)

#define z_pqueue_last(self)                                     \
  ((self)->objects + (((self)->tail - 1) * (self)->isize))

#define z_pqueue_peek(self)                                     \
  (z_pqueue_is_not_empty(self) ? z_pqueue_first(self) : NULL)

int       z_pqueue_open     (z_pqueue_t *self,
                             uint16_t isize,
                             void *objects,
                             uint32_t length);
void      z_pqueue_add      (z_pqueue_t *self, void *elem, z_compare_t key_cmp, void *udata);
uint8_t * z_pqueue_add_slot (z_pqueue_t *self, void *elem, z_compare_t key_cmp, void *udata);
uint8_t * z_pqueue_pop      (z_pqueue_t *self);

__Z_END_DECLS__

#endif /* _Z_PQUEUE_H_ */
