/*
 *   Copyright 2011 Matteo Bertozzi
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

#include <zcl/thread.h>

/* ============================================================================
 *  Plugin call related
 */
#define __plug_has_method(fs, type, method)                                 \
    ((fs)->type.plug != NULL && (fs)->type.plug->method != NULL)

#define __plug_call(fs, type, method, ...)                                  \
    ((fs)->type.plug->method(fs, ##__VA_ARGS__))

#define __plug_call_required(fs, type, method, ...)                         \
    (Z_LIKELY(__plug_has_method(fs, type, method)) ?                        \
        __plug_call(fs, type, method, ##__VA_ARGS__) :                      \
        RALEIGHFS_ERRNO_NOT_IMPLEMENTED)

#define __plug_call_unrequired(fs, type, method, ...)                       \
    (Z_LIKELY(__plug_has_method(fs, type, method)) ?                        \
        __plug_call(fs, type, method, ##__VA_ARGS__) :                      \
        RALEIGHFS_ERRNO_NONE)

#define __object_has_method(obj, method)                                    \
    ((obj)->internal->plug->method != NULL)

#define __object_call(fs, obj, method, ...)                                 \
    ((obj)->internal->plug->method(fs, obj, ##__VA_ARGS__))

#define __object_call_required(fs, obj, method, ...)                        \
    (Z_LIKELY(__object_has_method(obj, method)) ?                           \
        __object_call(fs, obj, method, ##__VA_ARGS__) :                     \
        RALEIGHFS_ERRNO_NOT_IMPLEMENTED)

#define __object_call_unrequired(fs, obj, method, ...)                      \
    (Z_LIKELY(__object_has_method(obj, method)) ?                           \
        __object_call(fs, obj, method, ##__VA_ARGS__) :                     \
        RALEIGHFS_ERRNO_NONE)

#define __device_has_method(fs, method)                                     \
    ((fs)->device->plug != NULL && (fs)->device->plug->method != NULL)

#define __device_call(fs, method, ...)                                      \
    ((fs)->device->plug->method(fs, ##__VA_ARGS__))

#define __device_call_required(fs, method, ...)                             \
    (Z_LIKELY(__device_has_method(fs, method)) ?                            \
        __device_call(fs, method, ##__VA_ARGS__) :                          \
        RALEIGHFS_ERRNO_NOT_IMPLEMENTED)

#define __device_call_unrequired(fs, method, ...)                           \
    (Z_LIKELY(__device_has_method(fs, method)) ?                            \
        __device_call(fs, method, ##__VA_ARGS__) :                          \
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


#define __objcache_call(fs, method, ...)                                    \
    __plug_call(fs, objcache, method, ##__VA_ARGS__)

#define __objcache_insert(fs, object)                                       \
    __objcache_call(fs, insert, object)

#define __objcache_remove(fs, object)                                       \
    __objcache_call(fs, remove, object)

#define __objcache_lookup(fs, object, key)                                  \
    __objcache_call(fs, lookup, object, key)

/* ============================================================================
 *  Plugin Table related
 */
extern raleighfs_objcache_plug_t raleighfs_objcache_tree;

int                 __plugin_table_alloc        (raleighfs_t *fs);
void                __plugin_table_free         (raleighfs_t *fs);

raleighfs_errno_t   __plugin_register           (raleighfs_t *fs,
                                                 raleighfs_plug_t *plug);
raleighfs_errno_t   __plugin_unregister         (raleighfs_t *fs,
                                                 raleighfs_plug_t *plug);

raleighfs_plug_t *  __plugin_lookup_by_uuid     (raleighfs_t *fs,
                                                 const uint8_t uuid[16]);
raleighfs_plug_t *  __plugin_lookup_by_label    (raleighfs_t *fs,
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
 *  Object related
 */
raleighfs_errno_t   __object_alloc      (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         const raleighfs_key_t *key);
raleighfs_errno_t   __object_copy       (raleighfs_t *fs,
                                         raleighfs_object_t *objdst,
                                         const raleighfs_key_t *new_key,
                                         const raleighfs_object_t *objsrc);
void                __object_free       (raleighfs_t *fs,
                                         raleighfs_object_t *object);

void                __object_set_key    (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         raleighfs_key_t *key);

/* TODO: Make this operations atomic! */
#define __object_inc_ref(object)        (++((object)->internal->refs))
#define __object_dec_ref(object)        (--((object)->internal->refs))

/* ============================================================================
 *  Read/Write Lock related
 */
#define __rwlock_init(rwlock)                                               \
    do {                                                                    \
        z_spin_init(&((rwlock)->lock));                                     \
        (rwlock)->access_count = 0;                                         \
        (rwlock)->wr_waiting = 0;                                           \
    } while (0)

#define __rwlock_uninit(rwlock)                                             \
    do {                                                                    \
        z_spin_destroy(&((rwlock)->lock));                                  \
    } while (0)

/* ============================================================================
 *  Observers related
 */
void    __observer_notify_create        (raleighfs_t *fs,
                                         const z_rdata_t *name);
void    __observer_notify_open          (raleighfs_t *fs,
                                         const z_rdata_t *name);
void    __observer_notify_rename        (raleighfs_t *fs,
                                         const z_rdata_t *old_name,
                                         const z_rdata_t *new_name);
void    __observer_notify_unlink        (raleighfs_t *fs,
                                         const z_rdata_t *name);

void    __observer_notify_insert        (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);
void    __observer_notify_update        (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);
void    __observer_notify_remove        (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);
void    __observer_notify_ioctl         (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);

#endif /* !_RALEIGHFS_PRIVATE_H_ */

