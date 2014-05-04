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

#include <zcl/global.h>
#include <zcl/atomic.h>
#include <zcl/bytes.h>

/* ============================================================================
 *  PUBLIC bytes methods
 */
z_bytes_t *z_bytes_alloc (size_t size) {
  z_bytes_t *self;
  uint8_t *buf;

  buf = z_memory_alloc(z_global_memory(), uint8_t, sizeof(z_bytes_t) + size + 1);
  if (Z_MALLOC_IS_NULL(buf))
    return(NULL);

  self = Z_BYTES(buf);
  z_memslice_set(&(self->slice), buf + sizeof(z_bytes_t), size);
  buf[sizeof(z_bytes_t) + size] = '\0';
  self->refs = 1;
  return(self);
}

z_bytes_t *z_bytes_acquire (z_bytes_t *self) {
  if (self != NULL)
    z_atomic_inc(&(self->refs));
  return(self);
}

void z_bytes_free (z_bytes_t *self) {
  if (self == NULL)
    return;
  if (z_atomic_dec(&(self->refs)) > 0)
    return;

  z_memory_free(z_global_memory(), self);
}

z_bytes_t *z_bytes_from_data (const void *data, size_t size) {
  z_bytes_t *self;

  self = z_bytes_alloc(size);
  if (Z_MALLOC_IS_NULL(self))
    return(NULL);

  z_bytes_set(self, data, size);
  return(self);
}

/* ============================================================================
 *  PRIVATE vtable-refs methods
 */
static void __bytes_inc_ref (void *object) {
  z_bytes_acquire(Z_BYTES(object));
}

static void __bytes_dec_ref (void *object) {
  z_bytes_free(Z_BYTES(object));
}

const z_vtable_refs_t z_vtable_bytes_refs = {
  .inc_ref = __bytes_inc_ref,
  .dec_ref = __bytes_dec_ref,
};
