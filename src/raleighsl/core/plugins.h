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

#ifndef _RALEIGHSL_PLUGINS_H_
#define _RALEIGHSL_PLUGINS_H_

#include <raleighsl/errno.h>

#include <zcl/macros.h>
#include <zcl/bytes.h>

#define __RALEIGHSL_PLUGIN_OBJECT__     raleighsl_plug_t info;

#define RALEIGHSL_PLUGIN_CAST(t, x)     Z_CONST_CAST(raleighsl_##t##_plug_t, x)
#define RALEIGHSL_SEMANTIC_PLUG(x)      RALEIGHSL_PLUGIN_CAST(semantic, x)
#define RALEIGHSL_OBJECT_PLUG(x)        RALEIGHSL_PLUGIN_CAST(object, x)
#define RALEIGHSL_FORMAT_PLUG(x)        RALEIGHSL_PLUGIN_CAST(format, x)
#define RALEIGHSL_DEVICE_PLUG(x)        RALEIGHSL_PLUGIN_CAST(device, x)
#define RALEIGHSL_SPACE_PLUG(x)         RALEIGHSL_PLUGIN_CAST(space, x)
#define RALEIGHSL_KEY_PLUG(x)           RALEIGHSL_PLUGIN_CAST(key, x)

#define RALEIGHSL_TRANSACTION(x)        Z_CAST(raleighsl_transaction_t, x)
#define RALEIGHSL_OBJECT(x)             Z_CAST(raleighsl_object_t, x)
#define RALEIGHSL_DEVICE(x)             Z_CAST(raleighsl_device_t, x)
#define RALEIGHSL_PLUG(x)               Z_CAST(raleighsl_plug_t, x)
#define RALEIGHSL_KEY(x)                Z_CAST(raleighsl_key_t, x)
#define RALEIGHSL(fs)                   Z_CAST(raleighsl_t, fs)

#define RALEIGHSL_PLUG_UUID(x)          (RALEIGHSL_PLUG(x)->uuid)
#define RALEIGHSL_PLUG_LABEL(x)         (RALEIGHSL_PLUG(x)->label)

#define RALEIGHSL_OBJDATA_KEY(x)        (&(RALEIGHSL_OBJDATA(x)->key))
#define RALEIGHSL_OBJECT_KEY(x)         (&(RALEIGHSL_OBJECT(x)->internal->key))

Z_TYPEDEF_STRUCT(raleighsl_semantic_plug)
Z_TYPEDEF_STRUCT(raleighsl_object_plug)
Z_TYPEDEF_STRUCT(raleighsl_format_plug)
Z_TYPEDEF_STRUCT(raleighsl_device_plug)
Z_TYPEDEF_STRUCT(raleighsl_space_plug)
Z_TYPEDEF_STRUCT(raleighsl_key_plug)
Z_TYPEDEF_STRUCT(raleighsl_plug)

Z_TYPEDEF_STRUCT(raleighsl_transaction)
Z_TYPEDEF_STRUCT(raleighsl_txn_atom)
Z_TYPEDEF_STRUCT(raleighsl_journal)
Z_TYPEDEF_STRUCT(raleighsl_semantic)
Z_TYPEDEF_STRUCT(raleighsl_txn_mgr)
Z_TYPEDEF_STRUCT(raleighsl_object)
Z_TYPEDEF_STRUCT(raleighsl_master)
Z_TYPEDEF_STRUCT(raleighsl_device)
Z_TYPEDEF_STRUCT(raleighsl_block)
Z_TYPEDEF_STRUCT(raleighsl_key)
Z_TYPEDEF_STRUCT(raleighsl)

typedef enum raleighsl_plug_type {
  RALEIGHSL_PLUG_TYPE_SEMANTIC = 0x1,
  RALEIGHSL_PLUG_TYPE_OBJCACHE = 0x2,
  RALEIGHSL_PLUG_TYPE_OBJECT   = 0x3,
  RALEIGHSL_PLUG_TYPE_FORMAT   = 0x4,
  RALEIGHSL_PLUG_TYPE_DEVICE   = 0x5,
  RALEIGHSL_PLUG_TYPE_SPACE    = 0x6,
  RALEIGHSL_PLUG_TYPE_KEY      = 0x7,
} raleighsl_plug_type_t;

typedef enum raleighsl_key_type {
  RALEIGHSL_KEY_TYPE_OBJECT    = 0x00,
  RALEIGHSL_KEY_TYPE_METADATA  = 0x01,
  RALEIGHSL_KEY_TYPE_DATA      = 0x02,
  RALEIGHSL_KEY_TYPE_USER_DATA = 0xA0,
} raleighsl_key_type_t;

struct raleighsl_plug {
  const char *          label;            /* Plugin label */
  const char *          description;      /* Short plugin description */
  raleighsl_plug_type_t type;             /* Plugin type */
};

struct raleighsl_semantic_plug {
  __RALEIGHSL_PLUGIN_OBJECT__

  raleighsl_errno_t   (*init)         (raleighsl_t *fs);
  raleighsl_errno_t   (*load)         (raleighsl_t *fs);
  raleighsl_errno_t   (*unload)       (raleighsl_t *fs);
  raleighsl_errno_t   (*sync)         (raleighsl_t *fs);
  raleighsl_errno_t   (*commit)       (raleighsl_t *fs);

  raleighsl_errno_t   (*create)       (raleighsl_t *fs,
                                       const z_bytes_ref_t *name,
                                       uint64_t oid);
  raleighsl_errno_t   (*lookup)       (raleighsl_t *fs,
                                       const z_bytes_ref_t *name,
                                       uint64_t *oid);
  raleighsl_errno_t   (*unlink)       (raleighsl_t *fs,
                                       const z_bytes_ref_t *name,
                                       uint64_t *oid);
};

struct raleighsl_object_plug {
  __RALEIGHSL_PLUGIN_OBJECT__

  raleighsl_errno_t   (*create)       (raleighsl_t *fs,
                                       raleighsl_object_t *object);
  raleighsl_errno_t   (*open)         (raleighsl_t *fs,
                                       raleighsl_object_t *object);
  raleighsl_errno_t   (*close)        (raleighsl_t *fs,
                                       raleighsl_object_t *object);
  raleighsl_errno_t   (*unlink)       (raleighsl_t *fs,
                                       raleighsl_object_t *object);

  void                (*apply)        (raleighsl_t *fs,
                                       raleighsl_object_t *object,
                                       raleighsl_txn_atom_t *atom);
  void                (*revert)       (raleighsl_t *fs,
                                       raleighsl_object_t *object,
                                       raleighsl_txn_atom_t *atom);
  raleighsl_errno_t   (*commit)       (raleighsl_t *fs,
                                       raleighsl_object_t *object);

  raleighsl_errno_t   (*balance)      (raleighsl_t *fs,
                                       raleighsl_object_t *object);
  raleighsl_errno_t   (*sync)         (raleighsl_t *fs,
                                       raleighsl_object_t *object);
};

struct raleighsl_key_plug {
  __RALEIGHSL_PLUGIN_OBJECT__

  raleighsl_errno_t   (*init)         (raleighsl_t *fs);
  raleighsl_errno_t   (*load)         (raleighsl_t *fs);
  raleighsl_errno_t   (*unload)       (raleighsl_t *fs);

  int                 (*compare)      (raleighsl_t *fs,
                                       const raleighsl_key_t *a,
                                       const raleighsl_key_t *b);

  raleighsl_errno_t   (*object)       (raleighsl_t *fs,
                                       raleighsl_key_t *key,
                                       const z_byte_slice_t *name);
};

struct raleighsl_device_plug {
  __RALEIGHSL_PLUGIN_OBJECT__

  uint64_t            (*used)         (raleighsl_t *fs);
  uint64_t            (*free)         (raleighsl_t *fs);

  raleighsl_errno_t   (*sync)         (raleighsl_t *fs);

  /* TODO: Replace buffer with push/pop function, chunk like */
  raleighsl_errno_t   (*read)         (raleighsl_t *fs,
                                       uint64_t offset,
                                       void *buffer,
                                       unsigned int size);
  raleighsl_errno_t   (*write)        (raleighsl_t *fs,
                                       uint64_t offset,
                                       const void *buffer,
                                       unsigned int size);
};

struct raleighsl_format_plug {
  __RALEIGHSL_PLUGIN_OBJECT__

  raleighsl_errno_t   (*init)         (raleighsl_t *fs);
  raleighsl_errno_t   (*load)         (raleighsl_t *fs);
  raleighsl_errno_t   (*unload)       (raleighsl_t *fs);
  raleighsl_errno_t   (*sync)         (raleighsl_t *fs);

  const uint8_t *     (*semantic)     (raleighsl_t *fs);
  const uint8_t *     (*space)        (raleighsl_t *fs);
  const uint8_t *     (*key)          (raleighsl_t *fs);
};

struct raleighsl_space_plug {
  __RALEIGHSL_PLUGIN_OBJECT__

  raleighsl_errno_t   (*init)         (raleighsl_t *fs);
  raleighsl_errno_t   (*load)         (raleighsl_t *fs);
  raleighsl_errno_t   (*unload)       (raleighsl_t *fs);
  raleighsl_errno_t   (*sync)         (raleighsl_t *fs);

  raleighsl_errno_t   (*allocate)     (raleighsl_t *fs,
                                       uint64_t request,
                                       uint64_t *start,
                                       uint64_t *count);
  raleighsl_errno_t   (*release)      (raleighsl_t *fs,
                                       uint64_t start,
                                       uint64_t count);

  raleighsl_errno_t   (*available)    (raleighsl_t *fs,
                                       uint64_t start,
                                       uint64_t count);
  raleighsl_errno_t   (*occupied)     (raleighsl_t *fs,
                                       uint64_t start,
                                       uint64_t count);
};

#endif /* !_RALEIGHSL_PLUGINS_H_ */
