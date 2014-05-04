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

#ifndef _Z_MEMORY_H_
#define _Z_MEMORY_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/allocator.h>
#include <zcl/histogram.h>
#include <zcl/macros.h>
#include <zcl/debug.h>

Z_TYPEDEF_STRUCT(z_memory)

#define Z_MALLOC_IS_NULL(x)           (Z_UNLIKELY((x) == NULL))

struct z_memory {
  z_allocator_t *allocator;

  z_histogram_t histo;
  uint64_t histo_events[26];
  //uint64_t pool_alloc;
  //uint64_t sys_alloc;
};


typedef void (*z_mem_item_t) (void *udata, void *object);
typedef void (*z_mem_free_t) (void *udata, void *object);

z_memory_t *z_memory_open   (z_memory_t *self, z_allocator_t *allocator);
void        z_memory_close  (z_memory_t *memory);

void        z_memory_stats_dump (z_memory_t *self, FILE *stream);

/* basic alloc/realloc/free */
void *z_memory_raw_alloc   (z_memory_t *self, const char *type_name, size_t size);
void *z_memory_raw_realloc (z_memory_t *self, void *ptr, size_t size);

#define z_memory_alloc(self, type, size)                                      \
  Z_CAST(type, z_memory_raw_alloc(self, #type, size))

#define z_memory_realloc(self, type, ptr, size)                               \
  Z_CAST(type, z_memory_raw_realloc(self, ptr, size))

#define z_memory_free(self, ptr)                                              \
  z_allocator_free(self->allocator, ptr);

/* struct alloc/free */
#define z_memory_struct_alloc(self, type)                                     \
  Z_CAST(type, z_memory_raw_alloc(self, #type, sizeof(type)))

#define z_memory_struct_free(self, type, ptr)                                 \
  z_memory_free(self, ptr)

/* array alloc/realloc/free */
#define z_memory_array_alloc(self, type, n)                                   \
  Z_CAST(type, z_memory_raw_alloc(self, #type "[]", (n) * sizeof(type)))

#define z_memory_array_realloc(self, ptr, type, n)                            \
  z_memory_realloc(self, type, ptr, (n) * sizeof(type))

#define z_memory_array_free(self, ptr)                                        \
  z_memory_free(self, ptr)

#if __Z_DEBUG__
  #define z_memory_set_dirty_debug(ptr, size)     z_memset(ptr, 0xff, size)
#else
  #define z_memory_set_dirty_debug(ptr, size)     while (0)
#endif

__Z_END_DECLS__

#endif /* _Z_MEMORY_H_ */
