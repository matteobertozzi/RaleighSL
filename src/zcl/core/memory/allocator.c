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

static const z_vtable_type_t __sys_allocator_type = {
  .name = "SysAllocator",
  .size = sizeof(z_allocator_t),
  .ctor = z_type_noop_ctor,
  .dtor = z_type_noop_dtor,
};

static const z_vtable_allocator_t __sys_allocator = {
  .alloc   = __system_alloc,
  .realloc = __system_realloc,
  .free    = __system_free,
};

static const z_allocator_interfaces_t __sys_allocator_interfafces = {
  .type      = &__sys_allocator_type,
  .allocator = &__sys_allocator,
};

int z_system_allocator_open (z_allocator_t *allocator) {
  allocator->vtables = &__sys_allocator_interfafces;
  return(0);
}
