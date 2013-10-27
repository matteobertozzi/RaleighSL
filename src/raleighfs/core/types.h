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

#ifndef _RALEIGHFS_TYPES_H_
#define _RALEIGHFS_TYPES_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/hashmap.h>
#include <zcl/object.h>
#include <zcl/opaque.h>
#include <zcl/cache.h>
#include <zcl/task.h>

#include <raleighfs/plugins.h>

#define RALEIGHFS_MASTER_MAGIC            ("R4l3igHfS-v5")
#define RALEIGHFS_MASTER_QMAGIC           (0xf5ba5028cb6afc76ul)

#define raleighfs_oid(obj)                z_cache_entry_oid(obj)
#define raleighfs_txn_id(txn)             z_cache_entry_oid(txn)

struct raleighfs_master {
  uint8_t  mb_magic[12];                  /* Master block magic */

  uint32_t mb_format;                     /* File-system format in use */
  uint64_t mb_ctime;                      /* File-system creation time */

  uint8_t  mb_uuid[16];                   /* File-system 128-bit uuid in use */
  uint8_t  mb_label[16];                  /* Files-ystem label in use */

  uint64_t mb_qmagic;                     /* Master block end-magic */
} __attribute__((__packed__));

struct raleighfs_key {
  uint64_t body[4];
};

typedef enum raleighfs_txn_state {
  RALEIGHFS_TXN_WAIT_COMMIT,
  RALEIGHFS_TXN_DONT_COMMIT,
  RALEIGHFS_TXN_COMMITTED,
  RALEIGHFS_TXN_ROLLEDBACK,
} raleighfs_txn_state_t;

struct raleighfs_transaction {
  __Z_CACHE_ENTRY__

  z_task_rwcsem_t rwcsem;                 /* Transaction RWC-Task-Lock */

  void *objects;                          /* TXN Object groups */
  uint64_t mtime;                         /* Modification time */

  raleighfs_txn_state_t state;            /* TXN state */
};

struct raleighfs_object {
  __Z_CACHE_ENTRY__

  z_task_rwcsem_t rwcsem;                 /* Object RWC-Task-Lock */
  uint64_t pending_txn_id;                /* Pending Transaction Id */

  const raleighfs_object_plug_t *plug;    /* Object plugin */

  void *devbufs;                          /* Object Device buffers */
  void *membufs;                          /* Object Memory buffers */
};

struct raleighfs_semantic {
  z_task_rwcsem_t rwcsem;                 /* Semantic RWC-Task-Lock */

  const raleighfs_semantic_plug_t *plug;  /* Semantic plugin */

  uint64_t next_oid;                      /* Next Object-ID */
  void *devbufs;                          /* Semantic Device buffers */
  void *membufs;                          /* Semantic Memory buffers */
};

struct raleighfs_device {
  const raleighfs_device_plug_t *plug;    /* Device plugin */
};

struct raleighfs {
  #define __RALEIGHFS_DECLARE_PLUG(name)                \
    struct {                                            \
      const raleighfs_ ## name ## _plug_t *plug;        \
      z_opaque_t data;                                  \
    } name;

  /* File-system plugins */
  __RALEIGHFS_DECLARE_PLUG(format)        /* Disk Format plug */
  __RALEIGHFS_DECLARE_PLUG(space)         /* Space Allocator */
  /* OID Allocator */
  /* Journal Layer */

  #undef __RALEIGHFS_DECLARE_PLUG

  raleighfs_semantic_t  semantic;         /* Semantic Layer */
  raleighfs_txn_mgr_t * txn_mgr;          /* Transaction Manager */

  z_cache_t *           obj_cache;
  raleighfs_device_t *  device;
  z_hash_map_t          plugins;
  raleighfs_master_t    master;
};

__Z_END_DECLS__

#endif /* !_RALEIGHFS_TYPES_H_ */