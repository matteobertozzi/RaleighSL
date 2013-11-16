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

#ifndef _Z_BUCKET_H_
#define _Z_BUCKET_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/byteslice.h>
#include <zcl/iterator.h>
#include <zcl/macros.h>
#include <zcl/map.h>

#define Z_CONST_BUCKET_ITERATOR(x)          Z_CONST_CAST(z_bucket_iterator_t, x)
#define Z_BUCKET_ITERATOR(x)                Z_CAST(z_bucket_iterator_t, x)

Z_TYPEDEF_STRUCT(z_vtable_bucket)

Z_TYPEDEF_STRUCT(z_bucket_iterator)
Z_TYPEDEF_STRUCT(z_bucket_entry)

struct z_bucket_entry {
  z_byte_slice_t key;
  z_byte_slice_t value;
  uint32_t kprefix;
  uint32_t index;
  int is_deleted;
};

struct z_bucket_iterator {
  __Z_MAP_ITERABLE__
  const z_vtable_bucket_t *vtable;
  z_map_entry_t map_entry;
  z_bucket_entry_t entry;
  uint8_t kbuffer[128];
  const uint8_t *node;
  int has_data;
};

struct z_vtable_bucket {
  int       (*open)         (const uint8_t *node, uint32_t size);
  void      (*create)       (uint8_t *node, uint32_t size);
  void      (*finalize)     (uint8_t *node);

  uint32_t  (*available)    (const uint8_t *node);
  int       (*has_space)    (const uint8_t *node,
                             const z_bucket_entry_t *item);
  int       (*append)       (uint8_t *node,
                             const z_bucket_entry_t *item);
  void      (*remove)       (uint8_t *node,
                             z_bucket_entry_t *entry);

  int       (*fetch_first)  (const uint8_t *node,
                             z_bucket_entry_t *entry);
  int       (*fetch_next)   (const uint8_t *node,
                             z_bucket_entry_t *entry);
};

extern const z_vtable_map_iterator_t z_bucket_map_iterator;
extern const z_vtable_bucket_t z_bucket_variable;

int z_bucket_search (const z_vtable_bucket_t *vtable,
                     const uint8_t *node,
                     const z_byte_slice_t *key,
                     z_byte_slice_t *value);

void z_bucket_iterator_open (z_bucket_iterator_t *self,
                             const z_vtable_bucket_t *vtable,
                             const uint8_t *node,
                             const z_vtable_refs_t *vrefs,
                             void *object);

__Z_END_DECLS__

#endif /* !_Z_BUCKET_H_ */
