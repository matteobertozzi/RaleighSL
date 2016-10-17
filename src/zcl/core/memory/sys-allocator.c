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

#include <zcl/allocator.h>

static void *__sys_mmalloc (z_allocator_t *self, size_t size) {
  void *ptr;
  ptr = z_sys_alloc(size);
  if (Z_LIKELY(ptr != NULL)) {
    self->extern_alloc++;
    self->extern_used += size;
  }
  return(ptr);
}

static void __sys_mmfree (z_allocator_t *self, void *ptr, size_t size) {
  self->extern_used -= size;
  z_sys_free(ptr, size);
}

void z_sys_allocator_init (z_allocator_t *self) {
  self->mmalloc = __sys_mmalloc;
  self->mmfree  = __sys_mmfree;
  self->pool_alloc = 0;
  self->pool_used  = 0;
  self->extern_alloc = 0;
  self->extern_used  = 0;
  self->mmpool = NULL;
  self->udata  = NULL;
}
