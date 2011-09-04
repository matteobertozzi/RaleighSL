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

#include <raleighfs/types.h>

#include <zcl/memcmp.h>
#include <zcl/strcmp.h>
#include <zcl/hash.h>

struct info_label_lookup {
    raleighfs_plug_t *plugin;
    const char *label;
};

static unsigned int __plugin_uuid_hash (void *data, const void *key) {
    return(z_hash32_dek(RALEIGHFS_PLUG(key)->uuid, 16, 0));
}

static int __plugins_compare (void *data, const void *a, const void *b) {
    return(z_memcmp(RALEIGHFS_PLUG_UUID(a), RALEIGHFS_PLUG_UUID(b), 16));
}

static unsigned int __uuid_hash (void *data, const void *key) {
    return(z_hash32_dek(key, 16, 0));
}

static int __plugin_uuid_compare (void *data, const void *a, const void *b) {
    return(z_memcmp(RALEIGHFS_PLUG_UUID(a), b, 16));
}

static int __plugin_label_compare (void *data, void *plug) {
    struct info_label_lookup *info = (struct info_label_lookup *)data;

    if (!z_strcmp(info->label, RALEIGHFS_PLUG_LABEL(plug))) {
        info->plugin = RALEIGHFS_PLUG(plug);
        return(0);
    }

    return(1);
}

int __plugin_table_alloc (raleighfs_t *fs) {
    if (!z_hash_table_alloc(&(fs->plugins), z_object_memory(fs),
                            &z_hash_table_open, 16,
                            __plugin_uuid_hash,
                            __plugins_compare,
                            NULL, NULL,
                            NULL, NULL))
    {
        return(RALEIGHFS_ERRNO_NO_MEMORY);
    }
    return(RALEIGHFS_ERRNO_NONE);
}

void __plugin_table_free (raleighfs_t *fs) {
    z_hash_table_free(&(fs->plugins));
}

raleighfs_errno_t __plugin_register (raleighfs_t *fs,
                                     raleighfs_plug_t *plug)
{
    if (z_hash_table_insert(&(fs->plugins), (void *)plug))
        return(RALEIGHFS_ERRNO_NO_MEMORY);
    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t __plugin_unregister (raleighfs_t *fs,
                                       raleighfs_plug_t *plug)
{
    if (z_hash_table_remove(&(fs->plugins), plug))
        return(RALEIGHFS_ERRNO_PLUGIN_NOT_LOADED);
    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_plug_t *__plugin_lookup_by_uuid (raleighfs_t *fs,
                                           const uint8_t uuid[16])
{
    void *plug;

    plug = z_hash_table_lookup_custom(&(fs->plugins),
                                      __uuid_hash,
                                      __plugin_uuid_compare,
                                      uuid);
    return(RALEIGHFS_PLUG(plug));
}

raleighfs_plug_t *__plugin_lookup_by_label (raleighfs_t *fs,
                                            const char *label)
{
    struct info_label_lookup info;

    info.plugin = NULL;
    info.label = label;
    z_hash_table_foreach(&(fs->plugins), __plugin_label_compare, &info);
    return(info.plugin);
}

/* ============================================================================
 *  PUBLIC plug/unplug methods
 */
#define __DECLARE_PLUG_REGISTER(type)                                         \
    raleighfs_errno_t raleighfs_plug_##type(raleighfs_t *fs,                  \
                                            raleighfs_##type##_plug_t*plug)   \
    {                                                                         \
        return(__plugin_register(fs, RALEIGHFS_PLUG(plug)));                  \
    }

#define __DECLARE_PLUG_UNREGISTER(type)                                       \
    raleighfs_errno_t raleighfs_unplug_##type(raleighfs_t *fs,                \
                                              raleighfs_##type##_plug_t*plug) \
    {                                                                         \
        return(__plugin_unregister(fs, RALEIGHFS_PLUG(plug)));                \
    }

__DECLARE_PLUG_REGISTER(key)
__DECLARE_PLUG_REGISTER(space)
__DECLARE_PLUG_REGISTER(format)
__DECLARE_PLUG_REGISTER(object)
__DECLARE_PLUG_REGISTER(semantic)

__DECLARE_PLUG_UNREGISTER(key)
__DECLARE_PLUG_UNREGISTER(space)
__DECLARE_PLUG_UNREGISTER(format)
__DECLARE_PLUG_UNREGISTER(object)
__DECLARE_PLUG_UNREGISTER(semantic)

/* ============================================================================
 *  PUBLIC lookup by label methods
 */
#define __DECLARE_PLUG_LOOKUP(type, utype)                                    \
    raleighfs_##type##_plug_t *                                               \
    raleighfs_##type##_plug_lookup (raleighfs_t *fs, const char *label) {     \
        return(RALEIGHFS_##utype##_PLUG(__plugin_lookup_by_label(fs, label)));\
    }

__DECLARE_PLUG_LOOKUP(key, KEY)
__DECLARE_PLUG_LOOKUP(space, SPACE)
__DECLARE_PLUG_LOOKUP(format, FORMAT)
__DECLARE_PLUG_LOOKUP(object, OBJECT)
__DECLARE_PLUG_LOOKUP(semantic, SEMANTIC)

