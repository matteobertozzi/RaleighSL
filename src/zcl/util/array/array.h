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

#ifndef _Z_ARRAY_H_
#define _Z_ARRAY_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_array_node z_array_node_t;
typedef struct z_array_iter z_array_iter_t;
typedef struct z_array z_array_t;

struct z_array {
  uint32_t length;          /* length of the array */
  uint32_t lbase;           /* length of the base */
  uint16_t isize;           /* item size */
  uint16_t fanout;          /* number of children in a node */
  uint32_t iper_block;      /* number of items per block (node+slots) */
  uint32_t slot_items;      /* length of the slot */
  uint32_t node_items;      /* length of the node */
  uint32_t node_count;      /* total number of nodes */
  uint32_t slots_size;      /* total size occupied by the slots in the node */
  uint8_t *ibase;           /* base array initialized with 'capacity' */
  z_array_node_t *iroot;    /* blocks tree */
};

struct z_array_iter {
  z_array_node_t *stack[32];
  z_array_node_t *current;
  z_array_t *array;
  uint8_t *blob;
  uint32_t length;
  uint16_t height;
  uint16_t slot;
};

#define z_array_get_u8(self, index)    Z_UINT8_PTR_VALUE(z_array_get_ptr(self, index))
#define z_array_get_u16(self, index)   Z_UINT16_PTR_VALUE(z_array_get_ptr(self, index))
#define z_array_get_u32(self, index)   Z_UINT32_PTR_VALUE(z_array_get_ptr(self, index))
#define z_array_get_u64(self, index)   Z_UINT64_PTR_VALUE(z_array_get_ptr(self, index))
#define z_array_get(self, index, type) Z_CAST(type, z_array_get_ptr(self, index))

#define z_array_length(self)    (self)->length

int       z_array_open      (z_array_t *self,
                             uint16_t isize,
                             uint16_t fanout,
                             uint32_t sblock,
                             uint32_t capacity);
void      z_array_close     (z_array_t *self);

void      z_array_resize    (z_array_t *self, uint32_t count);

void      z_array_zero_all  (z_array_t *self);
void      z_array_set_all   (z_array_t *self, const void *value);

void      z_array_set_ptr   (z_array_t *self, uint32_t index, const void *value);
uint8_t * z_array_get_ptr   (const z_array_t *self, uint32_t index);

int   z_array_iter_init   (z_array_iter_t *self, z_array_t *array);
int   z_array_iter_next   (z_array_iter_t *self);

void  z_array_sort        (z_array_t *self, z_compare_t cmp_func, void *udata);
void  z_array_sort_slots  (z_array_t *self, z_compare_t cmp_func, void *udata);

__Z_END_DECLS__

#endif /* !_Z_ARRAY_H_ */
