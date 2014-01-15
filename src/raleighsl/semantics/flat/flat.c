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
#include <zcl/hash.h>
#include <zcl/debug.h>
#include <zcl/bytes.h>

#include <raleighsl/objects/sset.h>
#include "flat.h"

struct object_entry {
  uint64_t oid;
  uint8_t  key[32];
  unsigned int refs;
};

#define __OBJECT_ENTRY(x)         Z_CAST(struct object_entry, x)

/* ============================================================================
 *  PRIVATE Object Entry Macros
 */
#define __name_to_key(key, name)                                              \
  z_hash256_sha(key, (name)->slice.data, (name)->slice.size)

static struct object_entry *__object_entry_alloc (const z_bytes_ref_t *name, uint64_t oid) {
  struct object_entry *entry;

  entry = z_memory_struct_alloc(z_global_memory(), struct object_entry);
  if (Z_MALLOC_IS_NULL(entry))
    return(NULL);

  entry->oid = oid;
  __name_to_key(entry->key, name);
  entry->refs = 1;
  return(entry);
}

static void __sset_block_inc_ref (void *object) {
  z_atomic_inc(&(__OBJECT_ENTRY(object)->refs));
}

static void __sset_block_dec_ref (void *object) {
  if (z_atomic_dec(&(__OBJECT_ENTRY(object)->refs)) == 0) {
    z_memory_struct_free(z_global_memory(), struct object_entry, object);
  }
}

static const z_vtable_refs_t __entry_vtable_refs = {
  .inc_ref = __sset_block_inc_ref,
  .dec_ref = __sset_block_dec_ref,
};

/* ============================================================================
 *  Flat Semantic Plugin
 */
static raleighsl_errno_t __semantic_init (raleighsl_t *fs) {
  raleighsl_errno_t errno;

  errno = raleighsl_object_create(fs, &raleighsl_object_sset, RALEIGHSL_ROOT_OID);
  if (errno) return(errno);

  fs->semantic.root = raleighsl_obj_cache_get(fs, RALEIGHSL_ROOT_OID);
  if (Z_UNLIKELY(fs->semantic.root == NULL)) {
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __semantic_load (raleighsl_t *fs) {
  raleighsl_errno_t errno;

  fs->semantic.root = raleighsl_obj_cache_get(fs, RALEIGHSL_ROOT_OID);
  if (Z_UNLIKELY(fs->semantic.root == NULL))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  if ((errno = raleighsl_object_open(fs, fs->semantic.root))) {
    raleighsl_obj_cache_release(fs, fs->semantic.root);
    return(errno);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __semantic_unload (raleighsl_t *fs) {
  raleighsl_obj_cache_release(fs, fs->semantic.root);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __semantic_commit (raleighsl_t *fs) {
  return(fs->semantic.root->plug->commit(fs, fs->semantic.root));
}

static raleighsl_errno_t __semantic_create (raleighsl_t *fs,
                                            const z_bytes_ref_t *name,
                                            uint64_t oid)
{
  struct object_entry *entry;
  raleighsl_errno_t errno;
  z_bytes_ref_t value;
  z_bytes_ref_t key;

  entry = __object_entry_alloc(name, oid);
  if (Z_MALLOC_IS_NULL(entry)) {
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  z_bytes_ref_set_data(&key, entry->key, 32, &__entry_vtable_refs, entry);
  z_bytes_ref_set_data(&value, &(entry->oid), 8, &__entry_vtable_refs, entry);
  errno = raleighsl_sset_insert(fs, NULL, fs->semantic.root, 0, &key, &value);
  switch (errno) {
    case RALEIGHSL_ERRNO_NONE:
      break;
    case RALEIGHSL_ERRNO_DATA_KEY_EXISTS:
      z_bytes_ref_release(&key);
      return(RALEIGHSL_ERRNO_OBJECT_EXISTS);
    default:
      z_bytes_ref_release(&key);
      return(errno);
  }

  z_bytes_ref_release(&key);
  return(errno);
}

static raleighsl_errno_t __semantic_lookup (raleighsl_t *fs,
                                            const z_bytes_ref_t *name,
                                            uint64_t *oid)
{
  raleighsl_errno_t errno;
  z_bytes_ref_t key_ref;
  z_bytes_ref_t value;
  uint8_t key[32];

  __name_to_key(key, name);
  z_bytes_ref_set_data(&key_ref, key, 32, NULL, NULL);

  errno = raleighsl_sset_get(fs, NULL, fs->semantic.root, &key_ref, &value);
  switch (errno) {
    case RALEIGHSL_ERRNO_NONE:
      break;
    case RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND:
      return(RALEIGHSL_ERRNO_OBJECT_NOT_FOUND);
    default:
      return(errno);
  }

  Z_ASSERT(value.slice.size == 8, "Expected uint64_t found %u bytes", value.slice.size);
  z_memcpy(oid, value.slice.data, 8);
  z_bytes_ref_release(&value);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __semantic_unlink (raleighsl_t *fs,
                                            const z_bytes_ref_t *name,
                                            uint64_t *oid)
{
  struct object_entry *entry;
  raleighsl_errno_t errno;
  z_bytes_ref_t value;
  z_bytes_ref_t key;

  entry = __object_entry_alloc(name, 0);
  if (Z_MALLOC_IS_NULL(entry)) {
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  z_bytes_ref_set_data(&key, entry->key, 32, &__entry_vtable_refs, entry);
  errno = raleighsl_sset_remove(fs, NULL, fs->semantic.root, &key, &value);
  switch (errno) {
    case RALEIGHSL_ERRNO_NONE:
      break;
    case RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND:
      z_bytes_ref_release(&key);
      return(RALEIGHSL_ERRNO_OBJECT_NOT_FOUND);
    default:
      z_bytes_ref_release(&key);
      return(errno);
  }

  Z_ASSERT(value.slice.size == 8, "Expected uint64_t found %u bytes", value.slice.size);
  z_memcpy(oid, value.slice.data, 8);
  z_bytes_ref_release(&key);
  z_bytes_ref_release(&value);
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

  .create   = __semantic_create,
  .lookup   = __semantic_lookup,
  .unlink   = __semantic_unlink,
};
