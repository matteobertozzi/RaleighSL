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

#include <zcl/byteslice.h>
#include <zcl/string.h>
#include <zcl/writer.h>
#include <zcl/debug.h>
#include <zcl/map.h>

void z_dump_map_entry (FILE *stream, const z_map_entry_t *entry) {
  if (Z_LIKELY(entry != NULL)) {
    fprintf(stream, "key: ");
    z_dump_byte_slice(stream, &(entry->key));
    fprintf(stream, " value: ");
    z_dump_byte_slice(stream, &(entry->value));
    fprintf(stream, "\n");
  } else {
    fprintf(stream, "key: NULL value: NULL\n");
  }
}

void z_map_iterator_seek_to (z_map_iterator_t *self,
                             const z_byte_slice_t *key,
                             int include_key)
{
  if (key == NULL || z_byte_slice_is_empty(key)) {
    z_map_iterator_begin(self);
  } else {
    z_map_iterator_seek(self, key);
#if 0
    fprintf(stderr, "seek to: ");
    z_dump_map_entry(stderr, z_map_iterator_current(self));
    fprintf(stderr, "expected ");
    z_dump_byte_slice(stderr, key);
#endif
    if (!include_key) {
      z_map_iterator_next(self);
#if 0
      fprintf(stderr, "seek to next: ");
      z_dump_map_entry(stderr, z_map_iterator_current(self));
#endif
    }
  }
}