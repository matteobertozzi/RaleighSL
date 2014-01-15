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

#ifndef _RALEIGHSL_PRIVATE_H_
#define _RALEIGHSL_PRIVATE_H_

#include <raleighsl/types.h>

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
      RALEIGHSL_ERRNO_NOT_IMPLEMENTED)

#define __plug_call_unrequired(fs, type, method, ...)                       \
  (Z_LIKELY(__plug_has_method(fs, type, method)) ?                          \
      __plug_call(fs, type, method, ##__VA_ARGS__) :                        \
      RALEIGHSL_ERRNO_NONE)

#define __object_has_method(obj, method)                                    \
  ((obj)->plug->method != NULL)

#define __object_call(fs, obj, method, ...)                                 \
  ((obj)->plug->method(fs, obj, ##__VA_ARGS__))

#define __object_call_required(fs, obj, method, ...)                        \
  (Z_LIKELY(__object_has_method(obj, method)) ?                             \
      __object_call(fs, obj, method, ##__VA_ARGS__) :                       \
      RALEIGHSL_ERRNO_NOT_IMPLEMENTED)

#define __object_call_unrequired(fs, obj, method, ...)                      \
  (Z_LIKELY(__object_has_method(obj, method)) ?                             \
      __object_call(fs, obj, method, ##__VA_ARGS__) :                       \
      RALEIGHSL_ERRNO_NONE)

#define __device_has_method(fs, method)                                     \
  ((fs)->device->plug != NULL && (fs)->device->plug->method != NULL)

#define __device_call(fs, method, ...)                                      \
  ((fs)->device->plug->method(fs, ##__VA_ARGS__))

#define __device_call_required(fs, method, ...)                             \
  (Z_LIKELY(__device_has_method(fs, method)) ?                              \
      __device_call(fs, method, ##__VA_ARGS__) :                            \
      RALEIGHSL_ERRNO_NOT_IMPLEMENTED)

#define __device_call_unrequired(fs, method, ...)                           \
  (Z_LIKELY(__device_has_method(fs, method)) ?                              \
      __device_call(fs, method, ##__VA_ARGS__) :                            \
      RALEIGHSL_ERRNO_NONE)

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
int                 __plugin_table_alloc        (raleighsl_t *fs);
void                __plugin_table_free         (raleighsl_t *fs);

raleighsl_errno_t   __plugin_register           (raleighsl_t *fs,
                                                 raleighsl_plug_t *plug);
raleighsl_errno_t   __plugin_unregister         (raleighsl_t *fs,
                                                 raleighsl_plug_t *plug);

const raleighsl_plug_t *__plugin_lookup_by_uuid  (raleighsl_t *fs,
                                                  const uint8_t uuid[16]);
const raleighsl_plug_t *__plugin_lookup_by_label (raleighsl_t *fs,
                                                  const char *label);

#define __semantic_plugin_lookup(fs, uuid)                                  \
  RALEIGHSL_SEMANTIC_PLUG(__plugin_lookup_by_uuid(fs, uuid))

#define __object_plugin_lookup(fs, uuid)                                    \
  RALEIGHSL_OBJECT_PLUG(__plugin_lookup_by_uuid(fs, uuid))

#define __format_plugin_lookup(fs, uuid)                                    \
  RALEIGHSL_FORMAT_PLUG(__plugin_lookup_by_uuid(fs, uuid))

#define __space_plugin_lookup(fs, uuid)                                     \
  RALEIGHSL_SPACE_PLUG(__plugin_lookup_by_uuid(fs, uuid))

#define __key_plugin_lookup(fs, uuid)                                       \
  RALEIGHSL_KEY_PLUG(__plugin_lookup_by_uuid(fs, uuid))

/* ============================================================================
 *  Transaction related
 */
int  raleighsl_txn_mgr_alloc (raleighsl_t *fs);
void raleighsl_txn_mgr_free  (raleighsl_t *fs);

/* ============================================================================
 *  Journal related
 */
int  raleighsl_journal_alloc (raleighsl_t *fs);
void raleighsl_journal_free  (raleighsl_t *fs);

/* ============================================================================
 *  Semantic related
 */
int   raleighsl_semantic_alloc    (raleighsl_t *fs);
void  raleighsl_semantic_free     (raleighsl_t *fs);

#define raleighsl_semantic_commit(fs)    __semantic_call_required(fs, commit)

/* ============================================================================
 *  Object related
 */
#define raleighsl_object_commit(fs, object)       \
  __object_call_required(fs, object, commit)

#define raleighsl_object_apply(fs, object, mutation)     \
  __object_call_required(fs, object, apply, mutation)

#define raleighsl_object_revert(fs, object, mutation)     \
  __object_call_required(fs, object, revert, mutation)

/* ============================================================================
 *  Object Cache related
 */
int                 raleighsl_obj_cache_alloc   (raleighsl_t *fs);
void                raleighsl_obj_cache_free    (raleighsl_t *fs);

#endif /* !_RALEIGHSL_PRIVATE_H_ */
