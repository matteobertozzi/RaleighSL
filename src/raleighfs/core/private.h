/*
 *   Copyright 2007-2013 Matteo Bertozzi
 *
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

#ifndef _RALEIGHFS_PRIVATE_H_
#define _RALEIGHFS_PRIVATE_H_

#include <raleighfs/types.h>

/* ============================================================================
 *  Plugin call related
 */
#define __plug_has_method(fs, type, method)                                 \
  ((fs)->type.plug != NULL && (fs)->type.plug->method != NULL)

#define __plug_call(fs, type, method, ...)                                  \
  ((fs)->type.plug->method(fs, ##__VA_ARGS__))

#define __plug_call_required(fs, type, method, ...)                         \
  (Z_LIKELY(__plug_has_method(fs, type, method)) ?                          \
      __plug_call(fs, type, method, ##__VA_ARGS__) :                        \
      RALEIGHFS_ERRNO_NOT_IMPLEMENTED)

#define __plug_call_unrequired(fs, type, method, ...)                       \
  (Z_LIKELY(__plug_has_method(fs, type, method)) ?                          \
      __plug_call(fs, type, method, ##__VA_ARGS__) :                        \
      RALEIGHFS_ERRNO_NONE)

#define __object_has_method(obj, method)                                    \
  ((obj)->plug->method != NULL)

#define __object_call(fs, obj, method, ...)                                 \
  ((obj)->plug->method(fs, obj, ##__VA_ARGS__))

#define __object_call_required(fs, obj, method, ...)                        \
  (Z_LIKELY(__object_has_method(obj, method)) ?                             \
      __object_call(fs, obj, method, ##__VA_ARGS__) :                       \
      RALEIGHFS_ERRNO_NOT_IMPLEMENTED)

#define __object_call_unrequired(fs, obj, method, ...)                      \
  (Z_LIKELY(__object_has_method(obj, method)) ?                             \
      __object_call(fs, obj, method, ##__VA_ARGS__) :                       \
      RALEIGHFS_ERRNO_NONE)

#define __device_has_method(fs, method)                                     \
  ((fs)->device->plug != NULL && (fs)->device->plug->method != NULL)

#define __device_call(fs, method, ...)                                      \
  ((fs)->device->plug->method(fs, ##__VA_ARGS__))

#define __device_call_required(fs, method, ...)                             \
  (Z_LIKELY(__device_has_method(fs, method)) ?                              \
      __device_call(fs, method, ##__VA_ARGS__) :                            \
      RALEIGHFS_ERRNO_NOT_IMPLEMENTED)

#define __device_call_unrequired(fs, method, ...)                           \
  (Z_LIKELY(__device_has_method(fs, method)) ?                              \
      __device_call(fs, method, ##__VA_ARGS__) :                            \
      RALEIGHFS_ERRNO_NONE)

/* ============================================================================
 *  Specific plugin calls
 */
#define __semantic_call(fs, method, ...)                                    \
  __plug_call(fs, semantic, method, ##__VA_ARGS__)

#define __semantic_call_required(fs, method, ...)                           \
  __plug_call_required(fs, semantic, method, ##__VA_ARGS__)

#define __semantic_call_unrequired(fs, method, ...)                         \
  __plug_call_unrequired(fs, semantic, method, ##__VA_ARGS__)


#define __space_call(fs, method, ...)                                       \
  __plug_call(fs, space, method, ##__VA_ARGS__)

#define __space_call_required(fs, method, ...)                              \
  __plug_call_required(fs, space, method, ##__VA_ARGS__)

#define __space_call_unrequired(fs, method, ...)                            \
  __plug_call_unrequired(fs, space, method, ##__VA_ARGS__)


#define __format_call(fs, method, ...)                                      \
  __plug_call(fs, format, method, ##__VA_ARGS__)

#define __format_call_required(fs, method, ...)                             \
  __plug_call_required(fs, format, method, ##__VA_ARGS__)

#define __format_call_unrequired(fs, method, ...)                           \
  __plug_call_unrequired(fs, format, method, ##__VA_ARGS__)


#define __key_call(fs, method, ...)                                         \
  __plug_call(fs, key, method, ##__VA_ARGS__)

#define __key_call_required(fs, method, ...)                                \
  __plug_call_required(fs, key, method, ##__VA_ARGS__)

#define __key_call_unrequired(fs, method, ...)                              \
  __plug_call_unrequired(fs, key, method, ##__VA_ARGS__)

/* ============================================================================
 *  Plugin Table related
 */
int                 __plugin_table_alloc        (raleighfs_t *fs);
void                __plugin_table_free         (raleighfs_t *fs);

raleighfs_errno_t   __plugin_register           (raleighfs_t *fs,
                                                 raleighfs_plug_t *plug);
raleighfs_errno_t   __plugin_unregister         (raleighfs_t *fs,
                                                 raleighfs_plug_t *plug);

const raleighfs_plug_t *__plugin_lookup_by_uuid  (raleighfs_t *fs,
                                                  const uint8_t uuid[16]);
const raleighfs_plug_t *__plugin_lookup_by_label (raleighfs_t *fs,
                                                  const char *label);

#define __semantic_plugin_lookup(fs, uuid)                                  \
  RALEIGHFS_SEMANTIC_PLUG(__plugin_lookup_by_uuid(fs, uuid))

#define __object_plugin_lookup(fs, uuid)                                    \
  RALEIGHFS_OBJECT_PLUG(__plugin_lookup_by_uuid(fs, uuid))

#define __format_plugin_lookup(fs, uuid)                                    \
  RALEIGHFS_FORMAT_PLUG(__plugin_lookup_by_uuid(fs, uuid))

#define __space_plugin_lookup(fs, uuid)                                     \
  RALEIGHFS_SPACE_PLUG(__plugin_lookup_by_uuid(fs, uuid))

#define __key_plugin_lookup(fs, uuid)                                       \
  RALEIGHFS_KEY_PLUG(__plugin_lookup_by_uuid(fs, uuid))

/* ============================================================================
 *  Transaction related
 */
int raleighfs_txn_mgr_alloc (raleighfs_t *fs);
void raleighfs_txn_mgr_free (raleighfs_t *fs);

/* ============================================================================
 *  Semantic related
 */
int   raleighfs_semantic_alloc    (raleighfs_t *fs);
void  raleighfs_semantic_free     (raleighfs_t *fs);

#define raleighfs_semantic_commit(fs)    __semantic_call_required(fs, commit)
#define raleighfs_semantic_rollback(fs)  __semantic_call_required(fs, rollback)

/* ============================================================================
 *  Object related
 */
raleighfs_errno_t raleighfs_object_create (raleighfs_t *fs,
                                           const raleighfs_object_plug_t *plug,
                                           uint64_t oid);
raleighfs_errno_t raleighfs_object_open   (raleighfs_t *fs,
                                           raleighfs_object_t *object);
raleighfs_errno_t raleighfs_object_close  (raleighfs_t *fs,
                                           raleighfs_object_t *object);
raleighfs_errno_t raleighfs_object_sync   (raleighfs_t *fs,
                                           raleighfs_object_t *object);

#define raleighfs_object_is_open(fs, object)                  \
  ((object)->membufs != NULL || (object)->devbufs != NULL)

#define raleighfs_object_commit(fs, object)       \
  __object_call_required(fs, object, commit)

#define raleighfs_object_rollback(fs, object)     \
  __object_call_required(fs, object, rollback)

#define raleighfs_object_apply(fs, object, mutation)     \
  __object_call_required(fs, object, apply, mutation)

#define raleighfs_object_revert(fs, object, mutation)     \
  __object_call_required(fs, object, revert, mutation)

/* ============================================================================
 *  Object Cache related
 */
int                 raleighfs_obj_cache_alloc   (raleighfs_t *fs);
void                raleighfs_obj_cache_free    (raleighfs_t *fs);

raleighfs_object_t *raleighfs_obj_cache_get     (raleighfs_t *fs,
                                                 uint64_t oid);
void                raleighfs_obj_cache_release (raleighfs_t *fs,
                                                 raleighfs_object_t *object);

#endif /* !_RALEIGHFS_PRIVATE_H_ */
