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

#include <zcl/sbuffer.h>
#include <zcl/mem.h>

struct sbuf_head {
  uint16_t nitems;
  uint16_t used;
  uint16_t size;
  uint8_t  seqio;
  uint8_t  type;      /* |111111|11| */

  uint16_t kmin;
  uint16_t kmax;
  uint16_t vmin;
  uint16_t vmax;
};

typedef int      (*sbuf_key_cmp_t) (const uint8_t *sbuffer, uint16_t pos, const z_memslice_t *key);
typedef void     (*sbuf_insert_t)  (uint8_t *sbuffer, const z_memslice_t *key, const z_memslice_t *value);
typedef void     (*sbuf_lookup_t)  (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *value);
typedef uint16_t (*sbuf_replace_t) (uint8_t *sbuffer, uint16_t pos, const z_memslice_t *key, const z_memslice_t *value);
typedef void     (*sbuf_get_t)     (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *key, z_memslice_t *value);

struct vtable_sbuffer {
  sbuf_key_cmp_t key_cmp;
  sbuf_lookup_t  lookup;
  sbuf_insert_t  insert;
  sbuf_replace_t replace;
  sbuf_get_t     get;
  int  overhead;
};

#define __SBUFFER_HEAD(sbuffer)         ((struct sbuf_head *)sbuffer)

#define __sbuf_begin(sbuffer)           ((sbuffer) + sizeof(struct sbuf_head))
#define __sbuf_end(sbuffer, head)       ((sbuffer) + (head)->size)

#define __sbuf_avail(sbuffer, head)                                           \
  ((head)->size - ((head)->used + (head)->nitems * 2))

#define __sbuf_idx_u16(sbuffer, head, index)                                  \
  ((uint16_t *)(__sbuf_end(sbuffer, head) - 2 - ((index) << 1)))

#define __sbuf_entry_size(vtable, key, value)                                 \
  ((vtable)->overhead + (key)->size + (value)->size)

/* ===========================================================================
 *  PRIVATE Dynamic Key Comparer SBuffer methods
 */
static int __dkey_cmp (const uint8_t *sbuffer, uint16_t pos, const z_memslice_t *key) {
  const struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  const uint8_t *pbuf = sbuffer + pos;
  return(z_data_cmp(head->type >> 2,
                    pbuf + 2, Z_UINT16_PTR_VALUE(pbuf),
                    key->data, key->size));
}

/* ===========================================================================
 *  PRIVATE Fixed Key Comparer SBuffer methods
 */
static int __fkey_cmp (const uint8_t *sbuffer, uint16_t pos, const z_memslice_t *key) {
  const struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  const uint8_t *pbuf = sbuffer + pos;
  return(z_data_cmp(head->type >> 2, pbuf, head->kmax, key->data, key->size));
}

/* ===========================================================================
 *  PRIVATE Dynamic Key/Value SBuffer methods
 */
static void __dkey_dvalue_insert (uint8_t *sbuffer, const z_memslice_t *key, const z_memslice_t *value) {
  uint8_t *pbuf = sbuffer + __SBUFFER_HEAD(sbuffer)->used;

  *((uint16_t *)pbuf) = key->size;
  memcpy(pbuf + 2, key->data, key->size);
  pbuf += 2 + key->size;

  *((uint16_t *)pbuf) = value->size;
  memcpy(pbuf + 2, value->data, value->size);
}

static uint16_t __dkey_dvalue_replace (uint8_t *sbuffer, uint16_t pos, const z_memslice_t *key, const z_memslice_t *value) {
  uint8_t *pbuf = sbuffer + pos;
  uint16_t vsize;
  pbuf += Z_UINT16_PTR_VALUE(pbuf) + 2;
  vsize = Z_UINT16_PTR_VALUE(pbuf);
  if (value->size > vsize) {
    pos = __SBUFFER_HEAD(sbuffer)->used;
    __dkey_dvalue_insert(sbuffer, key, value);
  } else {
    *((uint16_t *)pbuf) = value->size;
    memcpy(pbuf + 2, value->data, value->size);
  }
  return(pos);
}

static void __dkey_dvalue_lookup (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *value) {
  const uint8_t *pbuf = sbuffer + pos;
  pbuf += Z_UINT16_PTR_VALUE(pbuf) + 2;
  z_memslice_set(value, pbuf + 2, Z_UINT16_PTR_VALUE(pbuf));
}

static void __dkey_dvalue_get (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *key, z_memslice_t *value) {
  const uint8_t *pbuf = sbuffer + pos;
  z_memslice_set(key, pbuf + 2, Z_UINT16_PTR_VALUE(pbuf));
  pbuf += 2 + key->size;
  z_memslice_set(value, pbuf + 2, Z_UINT16_PTR_VALUE(pbuf));
}

static const struct vtable_sbuffer __dkey_dvalue_vtable = {
  .key_cmp  = __dkey_cmp,
  .lookup   = __dkey_dvalue_lookup,
  .insert   = __dkey_dvalue_insert,
  .replace  = __dkey_dvalue_replace,
  .get      = __dkey_dvalue_get,
  .overhead = 4,
};

/* ===========================================================================
 *  PRIVATE Dynamic Key/Fixed Value SBuffer methods
 */
static void __dkey_fvalue_insert (uint8_t *sbuffer, const z_memslice_t *key, const z_memslice_t *value) {
  uint8_t *pbuf = sbuffer + __SBUFFER_HEAD(sbuffer)->used;

  *((uint16_t *)pbuf) = key->size;
  memcpy(pbuf + 2, key->data, key->size);
  pbuf += 2 + key->size;

  memcpy(pbuf, value->data, value->size);
}

static uint16_t __dkey_fvalue_replace (uint8_t *sbuffer, uint16_t pos, const z_memslice_t *key, const z_memslice_t *value) {
  uint8_t *pbuf = sbuffer + pos;
  pbuf += Z_UINT16_PTR_VALUE(pbuf) + 2;
  memcpy(pbuf, value->data, value->size);
  return(pos);
}

static void __dkey_fvalue_lookup (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *value) {
  const uint8_t *pbuf = sbuffer + pos;
  pbuf += Z_UINT16_PTR_VALUE(pbuf) + 2;
  z_memslice_set(value, pbuf + 2, __SBUFFER_HEAD(sbuffer)->vmax);
}

static void __dkey_fvalue_get (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *key, z_memslice_t *value) {
  const uint8_t *pbuf = sbuffer + pos;
  z_memslice_set(key, pbuf + 2, Z_UINT16_PTR_VALUE(pbuf));
  pbuf += 2 + key->size;
  z_memslice_set(value, pbuf, __SBUFFER_HEAD(sbuffer)->vmax);
}

static const struct vtable_sbuffer __dkey_fvalue_vtable = {
  .key_cmp  = __dkey_cmp,
  .lookup   = __dkey_fvalue_lookup,
  .insert   = __dkey_fvalue_insert,
  .replace  = __dkey_fvalue_replace,
  .get      = __dkey_fvalue_get,
  .overhead = 2,
};

/* ===========================================================================
 *  PRIVATE Fixed Key/Value SBuffer methods
 */
static void __fkey_fvalue_insert (uint8_t *sbuffer, const z_memslice_t *key, const z_memslice_t *value) {
  uint8_t *pbuf = sbuffer + __SBUFFER_HEAD(sbuffer)->used;
  memcpy(pbuf, key->data, key->size);
  memcpy(pbuf + key->size, value->data, value->size);
}

static uint16_t __fkey_fvalue_replace (uint8_t *sbuffer, uint16_t pos, const z_memslice_t *key, const z_memslice_t *value) {
  uint8_t *pbuf = sbuffer + pos;
  pbuf += __SBUFFER_HEAD(sbuffer)->kmax;
  memcpy(pbuf, value->data, value->size);
  return(pos);
}

static void __fkey_fvalue_lookup (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *value) {
  const uint8_t *pbuf = sbuffer + pos;
  z_memslice_set(value, pbuf + __SBUFFER_HEAD(sbuffer)->kmax, __SBUFFER_HEAD(sbuffer)->vmax);
}

static void __fkey_fvalue_get (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *key, z_memslice_t *value) {
  const uint8_t *pbuf = sbuffer + pos;
  z_memslice_set(key, pbuf, __SBUFFER_HEAD(sbuffer)->kmax);
  z_memslice_set(value, pbuf + key->size, __SBUFFER_HEAD(sbuffer)->vmax);
}

static const struct vtable_sbuffer __fkey_fvalue_vtable = {
  .key_cmp  = __fkey_cmp,
  .lookup   = __fkey_fvalue_lookup,
  .insert   = __fkey_fvalue_insert,
  .replace  = __fkey_fvalue_replace,
  .get      = __fkey_fvalue_get,
  .overhead = 0,
};

/* ===========================================================================
 *  PRIVATE Fixed Key/Dynamic Value SBuffer methods
 */
static void __fkey_dvalue_insert (uint8_t *sbuffer, const z_memslice_t *key, const z_memslice_t *value) {
  uint8_t *pbuf = sbuffer + __SBUFFER_HEAD(sbuffer)->used;

  memcpy(pbuf, key->data, key->size);
  pbuf += key->size;

  *((uint16_t *)pbuf) = value->size;
  memcpy(pbuf + 2, value->data, value->size);
}

static uint16_t __fkey_dvalue_replace (uint8_t *sbuffer, uint16_t pos, const z_memslice_t *key, const z_memslice_t *value) {
  uint8_t *pbuf = sbuffer + pos;
  uint16_t vsize;
  pbuf += __SBUFFER_HEAD(sbuffer)->kmax;
  vsize = Z_UINT16_PTR_VALUE(pbuf);
  if (value->size > vsize) {
    pos = __SBUFFER_HEAD(sbuffer)->used;
    __fkey_dvalue_insert(sbuffer, key, value);
  } else {
    *((uint16_t *)pbuf) = value->size;
    memcpy(pbuf + 2, value->data, value->size);
  }
  return(pos);
}

static void __fkey_dvalue_lookup (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *value) {
  const uint8_t *pbuf = sbuffer + pos;
  pbuf += __SBUFFER_HEAD(sbuffer)->kmax;
  z_memslice_set(value, pbuf + 2, Z_UINT16_PTR_VALUE(pbuf));
}

static void __fkey_dvalue_get (const uint8_t *sbuffer, uint16_t pos, z_memslice_t *key, z_memslice_t *value) {
  const uint8_t *pbuf = sbuffer + pos;
  z_memslice_set(key, pbuf, __SBUFFER_HEAD(sbuffer)->kmax);
  pbuf += key->size;
  z_memslice_set(value, pbuf + 2, Z_UINT16_PTR_VALUE(pbuf));
}

static const struct vtable_sbuffer __fkey_dvalue_vtable = {
  .key_cmp  = __fkey_cmp,
  .lookup   = __fkey_dvalue_lookup,
  .insert   = __fkey_dvalue_insert,
  .replace  = __fkey_dvalue_replace,
  .get      = __fkey_dvalue_get,
  .overhead = 2,
};

/* ===========================================================================
 *  PRIVATE SBuffer types map
 */
static const struct vtable_sbuffer *__sbuffer_vtables[] = {
  &__dkey_dvalue_vtable,
  &__dkey_fvalue_vtable,
  &__fkey_dvalue_vtable,
  &__fkey_fvalue_vtable,
};

/* ===========================================================================
 *  PRIVATE SBuffer methods
 */
static uint16_t *__sbuf_bsearch (const uint8_t *sbuffer,
                                 sbuf_key_cmp_t cmp_func,
                                 const z_memslice_t *key,
                                 uint16_t *index)
{
  const struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  unsigned int start;
  unsigned int end;

  start = 0;
  end = head->nitems;
  while (start < end) {
    unsigned int mid;
    uint16_t *item;
    int cmp;

    mid = (start + end) >> 1;
    item = __sbuf_idx_u16(sbuffer, head, mid);
    cmp = cmp_func(sbuffer, *item, key);
    if (cmp > 0) {
      end = mid;
    } else if (cmp < 0) {
      start = mid + 1;
    } else {
      *index = mid;
      return(item);
    }
  }

  *index = start;
  return(NULL);
}

/* ===========================================================================
 *  PUBLIC SBuffer methods
 */

void z_sbuffer_init (uint8_t *sbuffer,
                     uint16_t size,
                     z_data_type_t key_type,
                     uint16_t ksize,
                     uint16_t vsize)
{
  struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  head->nitems = 0;
  head->used = sizeof(struct sbuf_head);
  head->size = size;
  head->seqio = 1;
  head->type = (key_type << 2);

  if (ksize != 0) {
    head->type |= 2;
    head->kmin  = ksize;
    head->kmax  = ksize;
  } else {
    head->kmin = 0xffff;
    head->kmax = 0;
  }

  if (vsize != 0) {
    head->type |= 1;
    head->vmin  = vsize;
    head->vmax  = vsize;
  } else {
    head->vmin = 0xffff;
    head->vmax = 0;
  }
}


int sbuffer_add (uint8_t *sbuffer, const z_memslice_t *key, const z_memslice_t *value) {
  struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  const struct vtable_sbuffer *vtable;
  uint16_t *ikey;
  int cmp = -1;

  vtable = __sbuffer_vtables[head->type];

  if (Z_UNLIKELY(__sbuf_avail(sbuffer, head) < __sbuf_entry_size(vtable, key, value)))
    return(1);

  if (Z_LIKELY(head->nitems > 0)) {
    uint16_t *last_key;
    last_key = __sbuf_idx_u16(sbuffer, head, head->nitems - 1);
    cmp = vtable->key_cmp(sbuffer, *last_key, key);
  }

  if (cmp < 0) {
    ikey = __sbuf_idx_u16(sbuffer, head, head->nitems);
    vtable->insert(sbuffer, key, value);
    *ikey = head->used;

    ++head->nitems;
    head->used += __sbuf_entry_size(vtable, key, value);;
    head->seqio &= 1;
  } else if (cmp > 0) {
    uint16_t index;
    ikey = __sbuf_bsearch(sbuffer, vtable->key_cmp, key, &index);
    if (ikey != NULL) {
      uint16_t offset;
      offset = vtable->replace(sbuffer, *ikey, key, value);
      if (*ikey != offset) {
        head->used += __sbuf_entry_size(vtable, key, value);
        *ikey = offset;
      }
    } else {
      uint16_t *iend;

      vtable->insert(sbuffer, key, value);
      iend = __sbuf_idx_u16(sbuffer, head, head->nitems);
      ikey = __sbuf_idx_u16(sbuffer, head, index);
      memmove(iend, iend + 1, ((uint8_t *)ikey) - ((uint8_t *)iend));
      *ikey = head->used;

      ++head->nitems;
      head->used += __sbuf_entry_size(vtable, key, value);
      head->seqio = 0;
    }
  } else /* if (cmp == 0) */ {
    uint16_t offset;

    ikey = __sbuf_idx_u16(sbuffer, head, head->nitems - 1);
    offset = vtable->replace(sbuffer, *ikey, key, value);
    if (*ikey != offset) {
      head->used += __sbuf_entry_size(vtable, key, value);
      *ikey = offset;
    }
  }
  return(0);
}

int sbuffer_merge (uint8_t *sbuffer, const uint8_t *ubuffer) {
  return(1);
}

int z_sbuffer_remove (uint8_t *sbuffer, const z_memslice_t *key) {
  struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  const struct vtable_sbuffer *vtable;
  uint16_t *ikey;
  uint16_t index;

  vtable = __sbuffer_vtables[head->type & 0x3];
  ikey = __sbuf_bsearch(sbuffer, vtable->key_cmp, key, &index);
  if (ikey == NULL)
    return(1);

  if (--head->nitems > index) {
    uint16_t *iend;
    iend = __sbuf_idx_u16(sbuffer, head, head->nitems);
    memmove(iend + 1, iend + 0, ((uint8_t *)ikey) - ((uint8_t *)iend));
  } else {
    z_memslice_t value;
    vtable->lookup(sbuffer, *ikey, &value);
    head->used -= __sbuf_entry_size(vtable, key, &value);
  }

  return(0);
}

int z_sbuffer_get (const uint8_t *sbuffer,
                   unsigned int index,
                   z_memslice_t *key,
                   z_memslice_t *value)
{
  const struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  const struct vtable_sbuffer *vtable;
  const uint16_t *ikey;

  if (Z_UNLIKELY(index >= head->nitems))
    return(1);

  vtable = __sbuffer_vtables[head->type & 0x3];
  ikey = __sbuf_idx_u16(sbuffer, head, index);
  vtable->get(sbuffer, *ikey, key, value);
  return(1);
}

int sbuffer_lookup (const uint8_t *sbuffer, const z_memslice_t *key, z_memslice_t *value) {
  const struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  const struct vtable_sbuffer *vtable;
  const uint16_t *ikey;
  uint16_t index;

  vtable = __sbuffer_vtables[head->type & 0x3];
  ikey = __sbuf_bsearch(sbuffer, vtable->key_cmp, key, &index);
  if (ikey == NULL)
    return(0);

  vtable->lookup(sbuffer, *ikey, value);
  return(1);
}

void z_sbuffer_debug (const uint8_t *sbuffer, FILE *stream) {
  const struct sbuf_head *head = __SBUFFER_HEAD(sbuffer);
  const struct vtable_sbuffer *vtable;
  uint16_t pos;
  int i, j;

  vtable = __sbuffer_vtables[head->type & 0x3];

  fprintf(stream, "sbuffer %u\n", head->size);
  fprintf(stream, " head->nitems: %u\n", head->nitems);
  fprintf(stream, " head->used:   %u\n", head->used);
  fprintf(stream, " head->size:   %u\n", head->size);
  fprintf(stream, " head->avail:  %u\n", head->size - (head->used + head->nitems * 2));
  fprintf(stream, " head->seqio:  %u\n", head->seqio);
  fprintf(stream, " head->type:   %u (kfix %u vfix %u)\n",
          head->type, !!(head->type & 2), !!(head->type & 1));
  fprintf(stream, " head->kmin:   %u\n", head->kmin);
  fprintf(stream, " head->kmax:   %u\n", head->kmax);
  fprintf(stream, " head->vmin:   %u\n", head->vmin);
  fprintf(stream, " head->vmax:   %u\n", head->vmax);

  pos = sizeof(struct sbuf_head);
  for (i = 0; i < head->nitems; ++i) {
    z_memslice_t value;
    z_memslice_t key;

    vtable->get(sbuffer, pos, &key, &value);

    fprintf(stream, " offset:%-4u ", pos);
    fprintf(stream, "%u:", key.size);
    for (j = 0; j < key.size; ++j)
      putc(key.data[j], stream);

    fprintf(stream, " %u:", value.size);
    for (j = 0; j < value.size; ++j)
      putc(value.data[j], stream);

    fprintf(stream, "\n");

    pos += vtable->overhead + key.size + value.size;
  }

  fprintf(stream, " index: [");
  for (i = 0; i < head->nitems; ++i) {
    uint16_t *offset = __sbuf_idx_u16(sbuffer, head, i);
    fprintf(stream, " %ld:%u ", (sbuffer + head->size) - (uint8_t *)offset, *offset);
  }
  fprintf(stream, " ]\n");

  fprintf(stream, " keys:  [");
  for (i = 0; i < head->nitems; ++i) {
    uint16_t *offset = __sbuf_idx_u16(sbuffer, head, i);
    z_memslice_t value;
    z_memslice_t key;

    vtable->get(sbuffer, *offset, &key, &value);
    pos += vtable->overhead + key.size + value.size;

    fprintf(stream, " %u:", key.size);
    for (j = 0; j < key.size; ++j)
      putc(key.data[j], stream);
    putc(' ', stream);
  }
  fprintf(stream, " ]\n\n");
}
