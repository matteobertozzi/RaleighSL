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

#ifndef _Z_BUFFER_H_
#define _Z_BUFFER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/object.h>

#define Z_CONST_BUFFER(x)                Z_CONST_CAST(z_buffer_t, x)
#define Z_BUFFER(x)                      Z_CAST(z_buffer_t, x)

Z_TYPEDEF_STRUCT(z_buffer)

struct z_buffer {
  __Z_OBJECT__(z_type)
  uint8_t *block;
  size_t size;
};

#define z_buffer_ensure(self, n)     z_buffer_reserve(self, (self)->size + (n))
#define z_buffer_tail(self)          ((self)->block + (self)->size)
#define z_buffer_size(self)          ((self).size)

z_buffer_t * z_buffer_alloc      (z_buffer_t *self);
void        z_buffer_free       (z_buffer_t *self);

void        z_buffer_release    (z_buffer_t *self);

int         z_buffer_squeeze    (z_buffer_t *self);
int         z_buffer_reserve    (z_buffer_t *self, size_t reserve);

int         z_buffer_clear      (z_buffer_t *self);
int         z_buffer_truncate   (z_buffer_t *self, size_t size);
int         z_buffer_remove     (z_buffer_t *self, size_t index, size_t size);

int         z_buffer_set        (z_buffer_t *self, const void *blob, size_t size);
int         z_buffer_append     (z_buffer_t *self, const void *blob, size_t size);
int         z_buffer_prepend    (z_buffer_t *self, const void *blob, size_t size);
int         z_buffer_insert     (z_buffer_t *self,
                                size_t index,
                                const void *blob,
                                size_t size);
int         z_buffer_replace    (z_buffer_t *self,
                                size_t index,
                                size_t size,
                                const void *blob,
                                size_t n);

int         z_buffer_equal      (z_buffer_t *self, const void *blob, size_t size);
int         z_buffer_compare    (z_buffer_t *self, const void *blob, size_t size);

__Z_END_DECLS__

#endif /* _Z_BUFFER_H_ */
