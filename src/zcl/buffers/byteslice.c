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

#include <zcl/debug.h>
#include <zcl/byteslice.h>
#include <zcl/string.h>

int z_byte_slice_strcmp (const z_byte_slice_t *self, const char *str) {
  z_byte_slice_t other;
  z_byte_slice_set(&other, str, z_strlen(str));
  return(z_byte_slice_compare(self, &other));
}

int z_byte_slice_compare (const z_byte_slice_t *self, const z_byte_slice_t *other) {
  if (self->size != other->size) {
    int cmp = z_memcmp(self->data, other->data, z_min(self->size, other->size));
    return((cmp != 0) ? cmp : (self->size - other->size));
  }
  return(z_memcmp(self->data, other->data, self->size));
}