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

#ifndef _RALEIGHFS_PLUGINS_H_
#define _RALEIGHFS_PLUGINS_H_

#include <raleighfs/errno.h>

#include <zcl/messageq.h>
#include <zcl/macros.h>
#include <zcl/slice.h>

#define RALEIGHFS_PLUGIN_OBJECT         raleighfs_plug_t info;

#define RALEIGHFS_PLUGIN_CAST(t, x)     Z_CONST_CAST(raleighfs_##t##_plug_t, x)
#define RALEIGHFS_SEMANTIC_PLUG(x)      RALEIGHFS_PLUGIN_CAST(semantic, x)
#define RALEIGHFS_OBJCACHE_PLUG(x)      RALEIGHFS_PLUGIN_CAST(objcache, x)
#define RALEIGHFS_OBJECT_PLUG(x)        RALEIGHFS_PLUGIN_CAST(object, x)
#define RALEIGHFS_FORMAT_PLUG(x)        RALEIGHFS_PLUGIN_CAST(format, x)
#define RALEIGHFS_DEVICE_PLUG(x)        RALEIGHFS_PLUGIN_CAST(device, x)
#define RALEIGHFS_SPACE_PLUG(x)         RALEIGHFS_PLUGIN_CAST(space, x)
#define RALEIGHFS_KEY_PLUG(x)           RALEIGHFS_PLUGIN_CAST(key, x)

#define RALEIGHFS_OBJDATA(x)            Z_CAST(raleighfs_objdata_t, x)
#define RALEIGHFS_OBJECT(x)             Z_CAST(raleighfs_object_t, x)
#define RALEIGHFS_DEVICE(x)             Z_CAST(raleighfs_device_t, x)
#define RALEIGHFS_PLUG(x)               Z_CAST(raleighfs_plug_t, x)
#define RALEIGHFS_KEY(x)                Z_CAST(raleighfs_key_t, x)
#define RALEIGHFS(fs)                   Z_CAST(raleighfs_t, fs)

#define RALEIGHFS_PLUG_UUID(x)          (RALEIGHFS_PLUG(x)->uuid)
#define RALEIGHFS_PLUG_LABEL(x)         (RALEIGHFS_PLUG(x)->label)

#define RALEIGHFS_OBJDATA_KEY(x)        (&(RALEIGHFS_OBJDATA(x)->key))
#define RALEIGHFS_OBJECT_KEY(x)         (&(RALEIGHFS_OBJECT(x)->internal->key))

Z_TYPEDEF_CONST_STRUCT(raleighfs_semantic_plug)
Z_TYPEDEF_CONST_STRUCT(raleighfs_objcache_plug)
Z_TYPEDEF_CONST_STRUCT(raleighfs_object_plug)
Z_TYPEDEF_CONST_STRUCT(raleighfs_format_plug)
Z_TYPEDEF_CONST_STRUCT(raleighfs_device_plug)
Z_TYPEDEF_CONST_STRUCT(raleighfs_space_plug)
Z_TYPEDEF_CONST_STRUCT(raleighfs_key_plug)
Z_TYPEDEF_CONST_STRUCT(raleighfs_plug)

Z_TYPEDEF_STRUCT(raleighfs_plug_data)
Z_TYPEDEF_STRUCT(raleighfs_objdata)
Z_TYPEDEF_STRUCT(raleighfs_object)
Z_TYPEDEF_STRUCT(raleighfs_master)
Z_TYPEDEF_STRUCT(raleighfs_device)
Z_TYPEDEF_STRUCT(raleighfs_rwlock)
Z_TYPEDEF_STRUCT(raleighfs_key)
Z_TYPEDEF_STRUCT(raleighfs)

typedef enum raleighfs_plug_type {
    RALEIGHFS_PLUG_TYPE_SEMANTIC = 0x1,
    RALEIGHFS_PLUG_TYPE_OBJCACHE = 0x2,
    RALEIGHFS_PLUG_TYPE_OBJECT   = 0x3,
    RALEIGHFS_PLUG_TYPE_FORMAT   = 0x4,
    RALEIGHFS_PLUG_TYPE_DEVICE   = 0x5,
    RALEIGHFS_PLUG_TYPE_SPACE    = 0x6,
    RALEIGHFS_PLUG_TYPE_KEY      = 0x7,
} raleighfs_plug_type_t;

typedef enum raleighfs_key_type {
    RALEIGHFS_KEY_TYPE_OBJECT    = 0x00,
    RALEIGHFS_KEY_TYPE_METADATA  = 0x01,
    RALEIGHFS_KEY_TYPE_DATA      = 0x02,
    RALEIGHFS_KEY_TYPE_USER_DATA = 0xA0,
} raleighfs_key_type_t;

struct raleighfs_plug {
    const char *          label;            /* Plugin label */
    const char *          description;      /* Short plugin description */
    const uint8_t *       uuid;             /* Plugin Unique-ID */
    raleighfs_plug_type_t type;             /* Plugin type */
};

struct raleighfs_plug_data {
    void *   ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
};

struct raleighfs_semantic_plug {
    RALEIGHFS_PLUGIN_OBJECT

    raleighfs_errno_t   (*init)         (raleighfs_t *fs);
    raleighfs_errno_t   (*load)         (raleighfs_t *fs);
    raleighfs_errno_t   (*unload)       (raleighfs_t *fs);

    raleighfs_errno_t   (*create)       (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         const z_slice_t *name);
    raleighfs_errno_t   (*open)         (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*close)        (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*sync)         (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*unlink)       (raleighfs_t *fs,
                                         raleighfs_object_t *object);

    raleighfs_errno_t   (*rename)       (raleighfs_t *fs,
                                         const raleighfs_key_t *old_key,
                                         const z_slice_t *old_name,
                                         const raleighfs_key_t *new_key,
                                         const z_slice_t *new_name);
};

struct raleighfs_object_plug {
    RALEIGHFS_PLUGIN_OBJECT

    raleighfs_errno_t   (*create)       (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*open)         (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*close)        (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*sync)         (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*unlink)       (raleighfs_t *fs,
                                         raleighfs_object_t *object);

    raleighfs_errno_t   (*query)        (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);
    raleighfs_errno_t   (*insert)       (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);
    raleighfs_errno_t   (*update)       (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);
    raleighfs_errno_t   (*remove)       (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);
    raleighfs_errno_t   (*ioctl)        (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *message);
};

struct raleighfs_key_plug {
    RALEIGHFS_PLUGIN_OBJECT

    raleighfs_errno_t   (*init)         (raleighfs_t *fs);
    raleighfs_errno_t   (*load)         (raleighfs_t *fs);
    raleighfs_errno_t   (*unload)       (raleighfs_t *fs);

    int                 (*compare)      (raleighfs_t *fs,
                                         const raleighfs_key_t *a,
                                         const raleighfs_key_t *b);

    raleighfs_errno_t   (*object)       (raleighfs_t *fs,
                                         raleighfs_key_t *key,
                                         const z_slice_t *name);
};

struct raleighfs_device_plug {
    RALEIGHFS_PLUGIN_OBJECT

    uint64_t            (*used)         (raleighfs_t *fs);
    uint64_t            (*free)         (raleighfs_t *fs);

    raleighfs_errno_t   (*sync)         (raleighfs_t *fs);

    /* TODO: Replace buffer with push/pop function, chunk like */
    raleighfs_errno_t   (*read)         (raleighfs_t *fs,
                                         uint64_t offset,
                                         void *buffer,
                                         unsigned int size);
    raleighfs_errno_t   (*write)        (raleighfs_t *fs,
                                         uint64_t offset,
                                         const void *buffer,
                                         unsigned int size);
};

struct raleighfs_format_plug {
    RALEIGHFS_PLUGIN_OBJECT

    raleighfs_errno_t   (*init)         (raleighfs_t *fs);
    raleighfs_errno_t   (*load)         (raleighfs_t *fs);
    raleighfs_errno_t   (*unload)       (raleighfs_t *fs);
    raleighfs_errno_t   (*sync)         (raleighfs_t *fs);

    const uint8_t *     (*semantic)     (raleighfs_t *fs);
    const uint8_t *     (*space)        (raleighfs_t *fs);
    const uint8_t *     (*key)          (raleighfs_t *fs);
};

struct raleighfs_space_plug {
    RALEIGHFS_PLUGIN_OBJECT

    raleighfs_errno_t   (*init)         (raleighfs_t *fs);
    raleighfs_errno_t   (*load)         (raleighfs_t *fs);
    raleighfs_errno_t   (*unload)       (raleighfs_t *fs);
    raleighfs_errno_t   (*sync)         (raleighfs_t *fs);

    raleighfs_errno_t   (*allocate)     (raleighfs_t *fs,
                                         uint64_t request,
                                         uint64_t *start,
                                         uint64_t *count);
    raleighfs_errno_t   (*release)      (raleighfs_t *fs,
                                         uint64_t start,
                                         uint64_t count);

    raleighfs_errno_t   (*available)    (raleighfs_t *fs,
                                         uint64_t start,
                                         uint64_t count);
    raleighfs_errno_t   (*occupied)     (raleighfs_t *fs,
                                         uint64_t start,
                                         uint64_t count);
};

struct raleighfs_objcache_plug {
    RALEIGHFS_PLUGIN_OBJECT

    raleighfs_errno_t   (*open)         (raleighfs_t *fs);
    raleighfs_errno_t   (*close)        (raleighfs_t *fs);
    raleighfs_errno_t   (*sync)         (raleighfs_t *fs);

    raleighfs_errno_t   (*insert)       (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*remove)       (raleighfs_t *fs,
                                         raleighfs_object_t *object);
    raleighfs_errno_t   (*lookup)       (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         const raleighfs_key_t *key);
};

#endif /* !_RALEIGHFS_PLUGINS_H_ */

