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
#include <zcl/debug.h>

#include "private.h"

/* ============================================================================
 *  PRIVATE Object methods
 */
raleighfs_object_t *raleighfs_object_alloc (raleighfs_t *fs, uint64_t oid) {
  raleighfs_object_t *object;

  object = z_memory_struct_alloc(z_global_memory(), raleighfs_object_t);
  if (Z_MALLOC_IS_NULL(object))
    return(NULL);

  /* Initialize cache-entry attributes */
  z_cache_entry_init(Z_CACHE_ENTRY(object), oid);

  /* Initialize object attributes */
  z_task_rwcsem_open(&(object->rwcsem));
  object->pending_txn_id = 0;

  object->plug = NULL;
  object->devbufs = NULL;
  object->membufs = NULL;
  return(object);
}

void raleighfs_object_free (raleighfs_t *fs, raleighfs_object_t *object) {
  z_task_rwcsem_close(&(object->rwcsem));
  z_memory_struct_free(z_global_memory(), raleighfs_object_t, object);
}

/* ============================================================================
 *  PRIVATE Object-Cache methods
 */
static void __obj_cache_entry_free (void *fs, void *object) {
  raleighfs_object_close(RALEIGHFS(fs), RALEIGHFS_OBJECT(object));
  raleighfs_object_free(RALEIGHFS(fs), RALEIGHFS_OBJECT(object));
}

int raleighfs_obj_cache_alloc (raleighfs_t *fs) {
  fs->obj_cache = z_cache_alloc(Z_CACHE_2Q, 100000,
                                __obj_cache_entry_free,
                                NULL, fs);
  if (Z_MALLOC_IS_NULL(fs->obj_cache))
    return(1);
  return(0);
}

void raleighfs_obj_cache_free (raleighfs_t *fs) {
  z_cache_free(fs->obj_cache);
}

raleighfs_object_t *raleighfs_obj_cache_get (raleighfs_t *fs, uint64_t oid) {
  raleighfs_object_t *obj;

  obj = RALEIGHFS_OBJECT(z_cache_lookup(fs->obj_cache, oid));
  if (obj == NULL) {
    raleighfs_object_t *other_obj;

    /* Allocate the new object */
    obj = raleighfs_object_alloc(fs, oid);
    if (Z_MALLOC_IS_NULL(obj))
      return(NULL);

    other_obj = RALEIGHFS_OBJECT(z_cache_try_insert(fs->obj_cache, Z_CACHE_ENTRY(obj)));
    if (other_obj != NULL) {
      /* The object was already inserted */
      raleighfs_object_free(fs, obj);
      return(other_obj);
    }
  }

  return(obj);
}

void raleighfs_obj_cache_release (raleighfs_t *fs, raleighfs_object_t *object) {
  z_cache_release(fs->obj_cache, Z_CACHE_ENTRY(object));
}