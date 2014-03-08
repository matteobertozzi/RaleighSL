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

#include <zcl/int-iterator.h>

/* ============================================================================
 *  PRIVATE UInt pack Iterator methods
 */
static int __uint_pack_iter_begin (void *self) {
  z_uint_pack_iter_t *iter = (z_uint_pack_iter_t *)self;
  z_memslice_t *entry = &(iter->ihead.entry_buf);

  iter->ihead.entry_buf.uflags32 = z_uint_unpacker_next(&(iter->unpacker));
  iter->ihead.entry = entry;
  entry->data = Z_UINT8_PTR(&(iter->unpacker.value));
  return(0);
}

static int __uint_pack_iter_next (void *self) {
  z_uint_pack_iter_t *iter = (z_uint_pack_iter_t *)self;
  if (Z_LIKELY(iter->ihead.entry_buf.uflags32)) {
    z_memslice_t *entry = &(iter->ihead.entry_buf);
    iter->ihead.entry_buf.uflags32 = z_uint_unpacker_next(&(iter->unpacker));
    iter->ihead.entry = entry;
    entry->data = Z_UINT8_PTR(&(iter->unpacker.value));
    return(1);
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __uint_pack_iter_seek (void *self, unsigned int index) {
  z_uint_pack_iter_t *iter = (z_uint_pack_iter_t *)self;
  z_memslice_t *entry = &(iter->ihead.entry_buf);
  z_uint_unpacker_seek(&(iter->unpacker), index);
  iter->ihead.entry = entry;
  entry->data = Z_UINT8_PTR(&(iter->unpacker.value));
  return(0);
}

static const z_vtable_iterator_t __uint_pack_iterator = {
  .begin = __uint_pack_iter_begin,
  .next  = __uint_pack_iter_next,
  .seek  = __uint_pack_iter_seek,
};

/* ============================================================================
 *  PUBLIC UInt pack Iterator methods
 */
void z_uint_pack_iter_open (z_uint_pack_iter_t *self,
                            const uint8_t *buffer,
                            uint32_t size)
{
  z_iterator_init(&(self->ihead), &__uint_pack_iterator);
  self->ihead.entry_buf.size = 8;
  z_uint_unpacker_open(&(self->unpacker), buffer, size);
}
