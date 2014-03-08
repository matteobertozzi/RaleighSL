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

#ifndef _Z_ITERATOR_H_
#define _Z_ITERATOR_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/bytes-packing.h>
#include <zcl/memslice.h>
#include <zcl/comparer.h>
#include <zcl/macros.h>
#include <zcl/vtable.h>
#include <zcl/dlink.h>

Z_TYPEDEF_STRUCT(z_bytes_vec_iter)
Z_TYPEDEF_STRUCT(z_fixed_vec_iter)
Z_TYPEDEF_STRUCT(z_vtable_iterator)
Z_TYPEDEF_STRUCT(z_iterator)

struct z_vtable_iterator {
  int   (*begin)  (void *self);
  int   (*next)   (void *self);
  int   (*seek)   (void *self, unsigned int index);
};

struct z_iterator {
  const z_vtable_iterator_t *vtable;
  z_memslice_t entry_buf;
  const z_memslice_t *entry;
};

struct z_bytes_vec_iter {
  z_iterator_t ihead;
  unsigned int current;
  unsigned int size;
  const z_memslice_t **entries;
};

struct z_fixed_vec_iter {
  z_iterator_t ihead;
  unsigned int current;
  unsigned int size;
  unsigned int stride;
  unsigned int width;
  const uint8_t *entries;
};

#define z_iterator_init(self, iter_vtable)            \
  do {                                                \
    (self)->vtable = iter_vtable;                     \
    (self)->entry = NULL;                             \
  } while (0)

#define z_iterator_begin(self)          z_vtable_call(self, begin)
#define z_iterator_next(self)           z_vtable_call(self, next)
#define z_iterator_seek(self, index)    z_vtable_call(self, seek, index)
#define z_iterator_current(self)        (self)->entry

void z_bytes_vec_iter_open (z_bytes_vec_iter_t *self,
                            const z_memslice_t **entries,
                            unsigned int nentries);
void z_fixed_vec_iter_open (z_fixed_vec_iter_t *self,
                            const void *entries,
                            unsigned int nentries,
                            unsigned int stride,
                            unsigned int width);

__Z_END_DECLS__

#endif /* _Z_ITERATOR_H_ */
