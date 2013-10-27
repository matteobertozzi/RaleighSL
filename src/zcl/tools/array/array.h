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
#include <zcl/buffer.h>

Z_TYPEDEF_STRUCT(z_array_block)
Z_TYPEDEF_STRUCT(z_array)

#define Z_ARRAY_BLOCK_SIZE          (64 - sizeof(z_array_block_t *))
#define Z_ARRAY_BLOCK_PTR_SIZE      (Z_ARRAY_BLOCK_SIZE / sizeof(void *))

struct z_array_block {
  z_array_block_t *next;
  union {
    uint8_t data[Z_ARRAY_BLOCK_SIZE];
    const void *ptrs[Z_ARRAY_BLOCK_PTR_SIZE];
  } items;
};

struct z_array {
  z_array_block_t head;
  z_array_block_t *tail;
  size_t count;
  int type_size;
  int per_block;
};

#define z_array_get(self, type, index)                \
  ((const type *)z_array_get_raw(self, index))

#define z_array_get_ptr(self, type, index)            \
  ((const type *)z_array_get_raw_ptr(self, index))

int   z_array_open            (z_array_t *self, int type_size);
void  z_array_close           (z_array_t *self);

void  z_array_clear             (z_array_t *self);
void *z_array_push_back         (z_array_t *self);
int   z_array_push_back_copy    (z_array_t *self, const void *data);
int   z_array_push_back_ptr     (z_array_t *self, const void *ptr);
const void *z_array_get_raw     (const z_array_t *self, size_t index);
const void *z_array_get_raw_ptr (const z_array_t *self, size_t index);

__Z_END_DECLS__

#endif /* _Z_ARRAY_H_ */
