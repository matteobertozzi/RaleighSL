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

#include <zcl/memutil.h>
#include <zcl/humans.h>
#include <zcl/dblock.h>
#include <zcl/sort.h>

/* ============================================================================
 *  Overlap helpers
 */
z_dblock_overlap_t z_dblock_overlap (const z_dblock_vtable_t *a_vtable,
                                     uint8_t *a_block,
                                     const z_dblock_vtable_t *b_vtable,
                                     uint8_t *b_block)
{
  z_dblock_kv_t b_key;
  z_dblock_kv_t a_key;
  int cmp;

  /* a.first_key < b.last_key */
  a_vtable->first_key(a_block, &a_key);
  b_vtable->last_key(b_block, &b_key);
  cmp = z_mem_compare(a_key.key, a_key.klength, b_key.key, b_key.klength);

  /* |-- B --| |-- A --| */
  if (cmp > 0) return Z_DBLOCK_OVERLAP_NO_RIGHT;

  /* |-- B --|-- A --| */
  if (cmp == 0) return Z_DBLOCK_OVERLAP_YES_RIGHT;

  /* b.first_key < a.last_key */
  a_vtable->last_key(a_block, &a_key);
  b_vtable->first_key(b_block, &b_key);
  cmp = z_mem_compare(b_key.key, b_key.klength, a_key.key, a_key.klength);

  /* |-- A --| |-- B --| */
  if (cmp > 0) return Z_DBLOCK_OVERLAP_NO_LEFT;

  /* |-- A --|-- B --| */
  if (cmp == 0) return Z_DBLOCK_OVERLAP_YES_LEFT;

  return Z_DBLOCK_OVERLAP_YES;
}

z_dblock_overlap_t z_dblock_overlap_blk (const z_dblock_kv_t *a_skey,
                                         const z_dblock_kv_t *a_ekey,
                                         const z_dblock_vtable_t *vtable,
                                         uint8_t *block)
{
  z_dblock_kv_t b_key;
  int cmp;

  /* a.first_key < b.last_key */
  vtable->last_key(block, &b_key);
  cmp = z_mem_compare(a_skey->key, a_skey->klength, b_key.key, b_key.klength);

  /* |-- B --| |-- A --| */
  if (cmp > 0) return Z_DBLOCK_OVERLAP_NO_RIGHT;

  /* |-- B --|-- A --| */
  if (cmp == 0) return Z_DBLOCK_OVERLAP_YES_RIGHT;

  /* b.first_key < a.last_key */
  vtable->first_key(block, &b_key);
  cmp = z_mem_compare(b_key.key, b_key.klength, a_ekey->key, a_ekey->klength);

  /* |-- A --| |-- B --| */
  if (cmp > 0) return Z_DBLOCK_OVERLAP_NO_LEFT;

  /* |-- A --|-- B --| */
  if (cmp == 0) return Z_DBLOCK_OVERLAP_YES_LEFT;

  return Z_DBLOCK_OVERLAP_YES;
}

/* ============================================================================
 *  Sorted Index helpers
 */
struct dblock_idx_udata {
  const z_dblock_vtable_t *vtable;
  uint8_t *block;
};

static int __dblock_index32_cmp (void *udata, const void *a, const void *b) {
  struct dblock_idx_udata *u = Z_CAST(struct dblock_idx_udata, udata);
  uint32_t a_ptr = *Z_CONST_CAST(uint32_t, a);
  uint32_t b_ptr = *Z_CONST_CAST(uint32_t, b);
  z_dblock_kv_t a_key;
  z_dblock_kv_t b_key;

  /* TODO: cache a/b ptr? */
  u->vtable->get_iptr(u->block, a_ptr, &a_key);
  u->vtable->get_iptr(u->block, b_ptr, &b_key);
  return z_mem_compare(a_key.key, a_key.klength, b_key.key, b_key.klength);
}

void z_dblock_build_index32 (const z_dblock_vtable_t *vtable,
                             uint8_t *block,
                             uint32_t *idx_block)
{
  struct dblock_idx_udata udata;
  z_dblock_iter_t iter;
  uint32_t *p;

  /* build the index */
  p = idx_block;
  vtable->seek(&iter, block, Z_DBLOCK_SEEK_BEGIN, NULL);
  do {
    *p++ = vtable->seek_iptr(&iter);
  } while (vtable->seek_next(&iter));

  /* sort the index */
  udata.vtable = vtable;
  udata.block  = block;
  z_heap_sort(idx_block, p - idx_block, sizeof(uint32_t),
              __dblock_index32_cmp, &udata);
}

/* ============================================================================
 *  vtable helpers
 */
const z_dblock_vtable_t *z_dblock_vtable (int type) {
  switch (type) {

  }
  return(NULL);
}

/* ============================================================================
 *  Print helpers
 */
void z_dblock_dump (const z_dblock_vtable_t *vtable,
                    uint8_t *block,
                    FILE *stream)
{
  z_dblock_iter_t iter;
  z_dblock_kv_t kv;

  /* build the index */
  vtable->seek(&iter, block, Z_DBLOCK_SEEK_BEGIN, NULL);
  do {
    vtable->seek_item(&iter, &kv);
    z_human_dblock_print(stream, kv.key, kv.klength);
    fprintf(stream, " = ");
    z_human_dblock_print(stream, kv.value, kv.vlength);
  } while (vtable->seek_next(&iter));
}