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

#include <string.h>

#include <zcl/memslice.h>
#include <zcl/debug.h>
#include <zcl/mem.h>

#if 0
int z_memslice_compare (const z_memslice_t *self, const z_memslice_t *other) {
  if (self->size != other->size) {
    int cmp = z_memcmp(self->data, other->data, z_min(self->size, other->size));
    return((cmp != 0) ? cmp : (self->size - other->size));
  }
  return(z_memcmp(self->data, other->data, self->size));
}

int z_mem_compare (const void *a, size_t asize, const void *b, size_t bsize) {
  if (asize != bsize) {
    int cmp = z_memcmp(a, b, z_min(asize, bsize));
    return((cmp != 0) ? cmp : (asize - bsize));
  }
  return(z_memcmp(a, b, asize));
}
#endif