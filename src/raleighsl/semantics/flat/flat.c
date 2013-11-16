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

#include <zcl/skiplist.h>
#include <zcl/global.h>
#include <zcl/hash.h>
#include <zcl/debug.h>
#include <zcl/bytes.h>

#include "flat.h"

struct object_entry {
  uint64_t oid;
  uint8_t key[32];
};

#define __semantic_table(fs)      Z_CAST(z_skip_list_t, (fs)->semantic.membufs)

/* ============================================================================
 *  PRIVATE Object Entry Macros
 */
#define __name_to_key(key, name)                                              \
  z_hash256_sha(key, (name)->slice.data, (name)->slice.size)

#define __object_entry_get_func(func, fs, key)                                \
  (Z_CAST(struct object_entry, func(__semantic_table(fs),                     \
                                    __object_entry_name_compare, key)))

#define __object_entry_lookup(fs, key)                                        \
  __object_entry_get_func(z_skip_list_get_custom, fs, key)

#define __object_entry_remove(fs, key)                                        \
  __object_entry_get_func(z_skip_list_remove_custom, fs, key)

/* ============================================================================
 *  PRIVATE Object Entry methods
 */
static int __object_entry_compare (void *udata, const void *a, const void *b) {
  const struct object_entry *ea = (const struct object_entry *)a;
  const struct object_entry *eb = (const struct object_entry *)b;
  return(z_memcmp(ea->key, eb->key, 32));
}

static int __object_entry_name_compare (void *udata, const void *a, const void *key) {
  const struct object_entry *entry = (const struct object_entry *)a;
  return(z_memcmp(entry->key, key, 32));
}

static void __object_entry_free (void *udata, void *obj) {
  struct object_entry *entry = (struct object_entry *)obj;
  z_memory_struct_free(z_global_memory(), struct object_entry, entry);
}

static struct object_entry *__object_entry_alloc (const z_bytes_ref_t *name, uint64_t oid) {
  struct object_entry *entry;

  entry = z_memory_struct_alloc(z_global_memory(), struct object_entry);
  if (Z_MALLOC_IS_NULL(entry))
    return(NULL);

  entry->oid = oid;
  __name_to_key(entry->key, name);
  return(entry);
}

/* ============================================================================
 *  Flat Semantic Plugin
 */
raleighsl_errno_t __semantic_init (raleighsl_t *fs) {
  z_skip_list_t *table;

  table = z_skip_list_alloc(NULL, __object_entry_compare, __object_entry_free, fs, (size_t)fs);
  if (Z_MALLOC_IS_NULL(table)) {
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  fs->semantic.membufs = table;
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t __semantic_load (raleighsl_t *fs) {
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t __semantic_unload (raleighsl_t *fs) {
  z_skip_list_free(__semantic_table(fs));
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t __semantic_commit (raleighsl_t *fs) {
  z_skip_list_commit(__semantic_table(fs));
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t __semantic_rollback (raleighsl_t *fs) {
  z_skip_list_rollback(__semantic_table(fs));
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t __semantic_create (raleighsl_t *fs,
                                     const z_bytes_ref_t *name,
                                     uint64_t oid)
{
  struct object_entry *entry;

  entry = __object_entry_alloc(name, oid);
  if (Z_MALLOC_IS_NULL(entry)) {
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  if (z_skip_list_put(__semantic_table(fs), entry)) {
    __object_entry_free(fs, entry);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t __semantic_lookup (raleighsl_t *fs,
                                     const z_bytes_ref_t *name,
                                     uint64_t *oid)
{
  struct object_entry *entry;
  uint8_t key[32];

  __name_to_key(key, name);
  if ((entry = __object_entry_lookup(fs, key)) == NULL) {
    return(RALEIGHSL_ERRNO_OBJECT_NOT_FOUND);
  }

  *oid = entry->oid;
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t __semantic_unlink (raleighsl_t *fs,
                                     const z_bytes_ref_t *name,
                                     uint64_t *oid)
{
  struct object_entry *entry;
  uint8_t key[32];

  __name_to_key(key, name);
  entry = __object_entry_remove(fs, key);
  if (entry == NULL) {
    return(RALEIGHSL_ERRNO_OBJECT_NOT_FOUND);
  }

  *oid = entry->oid;
  return(RALEIGHSL_ERRNO_NONE);
}

const raleighsl_semantic_plug_t raleighsl_semantic_flat = {
  .info = {
    .type = RALEIGHSL_PLUG_TYPE_SEMANTIC,
    .description = "Flat Semantic",
    .label       = "semantic-flat",
  },

  .init     = __semantic_init,
  .load     = __semantic_load,
  .unload   = __semantic_unload,
  .sync     = NULL,
  .commit   = __semantic_commit,
  .rollback = __semantic_rollback,

  .create   = __semantic_create,
  .lookup   = __semantic_lookup,
  .unlink   = __semantic_unlink,
};
