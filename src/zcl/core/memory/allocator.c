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

#include <stdlib.h>

#include <zcl/allocator.h>
#include <zcl/debug.h>

/* ============================================================================
 *  System Allocator
 */
static void *__system_alloc (z_allocator_t *self, size_t size) {
  return(malloc(size));
}

static void *__system_realloc (z_allocator_t *self, void *ptr, size_t size) {
  return(realloc(ptr, size));
}

static void __system_free (z_allocator_t *self, void *ptr) {
  free(ptr);
}

static void __system_noop (z_allocator_t *self) {
}

static const z_vtable_allocator_t __sys_allocator = {
  .alloc   = __system_alloc,
  .realloc = __system_realloc,
  .free    = __system_free,
  .ctor    = NULL,
  .dtor    = __system_noop,
};

int z_system_allocator_open (z_allocator_t *self) {
  self->vtable = &__sys_allocator;
  self->data = NULL;
  return(0);
}
