/*
 *   Copyright 2007-2012 Matteo Bertozzi
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

#include <zcl/string.h>
#include <zcl/hash.h>

static uint64_t __plugin_label_hash (void *data, const void *key, uint32_t seed) {
  const char *label = RALEIGHFS_PLUG_LABEL(key);
  return(z_hash32_dek(label, z_strlen(label), seed));
}

static int __plugins_compare (void *data, const void *a, const void *b) {
  return(z_strcmp(RALEIGHFS_PLUG_LABEL(a), RALEIGHFS_PLUG_LABEL(b)));
}

static int __plugin_label_compare (void *data, const void *a, const void *b) {
  const z_byte_slice_t *b_label = (const z_byte_slice_t *)b;
  return(-z_byte_slice_strcmp(b_label, RALEIGHFS_PLUG_LABEL(a)));
}

int __plugin_table_alloc (raleighfs_t *fs) {
  if (!z_hash_map_alloc(&(fs->plugins),
                        &z_open_hash_map,
                        __plugin_label_hash,
                        __plugins_compare,
                        NULL, NULL, 0, 16))
  {
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }
  return(RALEIGHFS_ERRNO_NONE);
}

void __plugin_table_free (raleighfs_t *fs) {
  z_hash_map_free(&(fs->plugins));
}

raleighfs_errno_t __plugin_register (raleighfs_t *fs,
                                     raleighfs_plug_t *plug)
{
  if (z_hash_map_put(&(fs->plugins), plug))
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t __plugin_unregister (raleighfs_t *fs,
                                       raleighfs_plug_t *plug)
{
  if (z_hash_map_remove(&(fs->plugins), plug))
    return(RALEIGHFS_ERRNO_PLUGIN_NOT_LOADED);
  return(RALEIGHFS_ERRNO_NONE);
}


const raleighfs_plug_t *__plugin_lookup_by_label (raleighfs_t *fs,
                                                 const z_byte_slice_t *label)
{
  uint32_t hash = z_hash32_dek(label->data, label->length, fs->plugins.seed);
  return(RALEIGHFS_PLUG(z_hash_map_get_custom(&(fs->plugins),
                                             __plugin_label_compare,
                                             hash, label)));
}

/* ============================================================================
 *  PUBLIC plug/unplug methods
 */
#define __DECLARE_PLUG_REGISTER(type)                                         \
  raleighfs_errno_t raleighfs_plug_##type (raleighfs_t *fs,                   \
                                      const raleighfs_##type##_plug_t*plug)   \
  {                                                                           \
      return(__plugin_register(fs, RALEIGHFS_PLUG(plug)));                    \
  }

#define __DECLARE_PLUG_UNREGISTER(type)                                       \
  raleighfs_errno_t raleighfs_unplug_##type (raleighfs_t *fs,                 \
                                        const raleighfs_##type##_plug_t*plug) \
  {                                                                           \
      return(__plugin_unregister(fs, RALEIGHFS_PLUG(plug)));                  \
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
#define __DECLARE_PLUG_LOOKUP(type, utype)                                        \
  const raleighfs_##type##_plug_t *                                               \
  raleighfs_##type##_plug_lookup (raleighfs_t *fs, const z_byte_slice_t *label) { \
    return(RALEIGHFS_ ## utype ## _PLUG(__plugin_lookup_by_label(fs, label)));    \
  }

__DECLARE_PLUG_LOOKUP(key, KEY)
__DECLARE_PLUG_LOOKUP(space, SPACE)
__DECLARE_PLUG_LOOKUP(format, FORMAT)
__DECLARE_PLUG_LOOKUP(object, OBJECT)
__DECLARE_PLUG_LOOKUP(semantic, SEMANTIC)