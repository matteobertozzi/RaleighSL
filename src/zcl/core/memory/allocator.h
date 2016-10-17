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

#ifndef _Z_ALLOCATOR_H_
#define _Z_ALLOCATOR_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_allocator z_allocator_t;

struct z_allocator {
  void *  (*mmalloc)  (z_allocator_t *self, size_t size);
  void    (*mmfree)   (z_allocator_t *self, void *ptr, size_t size);

  uint64_t pool_alloc;
  uint64_t pool_used;
  uint64_t extern_alloc;
  uint64_t extern_used;

  uint8_t *mmpool;
  void *   udata;
};

#define z_allocator_raw_alloc(self, size)        (self)->mmalloc(self, size)
#define z_allocator_free(self, ptr, size)        (self)->mmfree(self, ptr, size)

#define z_allocator_alloc(self, type, size)                                   \
  Z_CAST(type, z_allocator_raw_alloc(self, size))

#define z_allocator_realloc(self, type, size)                                 \
  Z_CAST(type, z_allocator_raw_realloc(self, ptr, size))

void *z_raw_alloc (size_t size);
void  z_raw_free  (void *ptr, size_t size);

#define z_sys_alloc(size)             malloc(size)
#define z_sys_free(ptr, size)         free(ptr)

void z_sys_allocator_init (z_allocator_t *self);

__Z_END_DECLS__

#endif /* !_Z_ALLOCATOR_H_ */
