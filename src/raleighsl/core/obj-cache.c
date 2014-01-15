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

#include <raleighsl/object.h>

#include <zcl/global.h>
#include <zcl/debug.h>

#include "private.h"

/* ============================================================================
 *  PRIVATE Object macros
 */
#define __obj_from_cache_entry(entry)                         \
  z_container_of(entry, raleighsl_object_t, cache_entry)

/* ============================================================================
 *  PRIVATE Object methods
 */
raleighsl_object_t *raleighsl_object_alloc (raleighsl_t *fs, uint64_t oid) {
  raleighsl_object_t *object;

  object = z_memory_struct_alloc(z_global_memory(), raleighsl_object_t);
  if (Z_MALLOC_IS_NULL(object))
    return(NULL);

  /* Initialize cache-entry attributes */
  z_cache_entry_init(&(object->cache_entry), oid);

  /* Initialize object attributes */
  z_task_rwcsem_open(&(object->rwcsem));
  object->pending_txn_id = 0;

  object->plug = NULL;
  object->devbufs = NULL;
  object->membufs = NULL;

  z_dlink_init(&(object->journal));
  return(object);
}

void raleighsl_object_free (raleighsl_t *fs, raleighsl_object_t *object) {
  z_task_rwcsem_close(&(object->rwcsem));
  z_memory_struct_free(z_global_memory(), raleighsl_object_t, object);
}

/* ============================================================================
 *  PRIVATE Object-Cache methods
 */
static void __obj_cache_entry_free (void *udata, void *entry) {
  raleighsl_object_t *object = __obj_from_cache_entry(entry);
  raleighsl_t *fs = RALEIGHSL(udata);
  raleighsl_object_sync(fs, object);
  raleighsl_object_close(fs, object);
  raleighsl_object_free(fs, object);
}

int raleighsl_obj_cache_alloc (raleighsl_t *fs) {
  fs->obj_cache = z_cache_alloc(Z_CACHE_2Q, 100000,
                                __obj_cache_entry_free,
                                NULL, fs);
  if (Z_MALLOC_IS_NULL(fs->obj_cache))
    return(1);
  return(0);
}

void raleighsl_obj_cache_free (raleighsl_t *fs) {
  z_cache_free(fs->obj_cache);
}

raleighsl_object_t *raleighsl_obj_cache_get (raleighsl_t *fs, uint64_t oid) {
  z_cache_entry_t *entry;

  entry = z_cache_lookup(fs->obj_cache, oid);
  if (entry == NULL) {
    raleighsl_object_t *obj;

    /* Allocate the new object */
    obj = raleighsl_object_alloc(fs, oid);
    if (Z_MALLOC_IS_NULL(obj))
      return(NULL);

    entry = z_cache_try_insert(fs->obj_cache, &(obj->cache_entry));
    if (entry == NULL)
      return(obj);

    /* The object was already inserted */
    raleighsl_object_free(fs, obj);
  }

  return(__obj_from_cache_entry(entry));
}

void raleighsl_obj_cache_release (raleighsl_t *fs, raleighsl_object_t *object) {
  z_cache_release(fs->obj_cache, &(object->cache_entry));
}