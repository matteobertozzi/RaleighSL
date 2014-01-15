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

#ifndef _RALEIGHSL_TYPES_H_
#define _RALEIGHSL_TYPES_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/hashmap.h>
#include <zcl/object.h>
#include <zcl/opaque.h>
#include <zcl/ticket.h>
#include <zcl/cache.h>
#include <zcl/dlink.h>
#include <zcl/task.h>

#include <raleighsl/plugins.h>

#define RALEIGHSL_MASTER_MAGIC          ("R4l3igHfS-v5")
#define RALEIGHSL_MASTER_QMAGIC         (0xf5ba5028cb6afc76ul)

#define raleighsl_oid(obj)              z_cache_entry_oid(&((obj)->cache_entry))
#define raleighsl_txn_id(txn)           z_cache_entry_oid(&((txn)->cache_entry))

struct raleighsl_master {
  uint8_t  mb_magic[12];                  /* Master block magic */

  uint32_t mb_format;                     /* File-system format in use */
  uint64_t mb_ctime;                      /* File-system creation time */

  uint8_t  mb_uuid[16];                   /* File-system 128-bit uuid in use */
  uint8_t  mb_label[16];                  /* Files-ystem label in use */

  uint64_t mb_qmagic;                     /* Master block end-magic */
} __attribute__((__packed__));

struct raleighsl_key {
  uint64_t body[4];
};

typedef enum raleighsl_txn_state {
  RALEIGHSL_TXN_WAIT_COMMIT,
  RALEIGHSL_TXN_DONT_COMMIT,
  RALEIGHSL_TXN_COMMITTED,
  RALEIGHSL_TXN_ROLLEDBACK,
} raleighsl_txn_state_t;

struct raleighsl_txn_atom {
  raleighsl_txn_atom_t *next;
};

struct raleighsl_block {
  z_cache_entry_t cache_entry;

  z_tree_node_t hash_node;
  uint64_t hash[4];

  uint32_t length;
  uint32_t pad;

  uint8_t *data;
};

struct raleighsl_transaction {
  z_cache_entry_t cache_entry;            /* Transaction Cache Entry */

  z_task_rwcsem_t rwcsem;                 /* Transaction RWC-Task-Lock */

  void *objects;                          /* Transaction Object groups */
  uint64_t mtime;                         /* Modification time */

  raleighsl_txn_state_t state;            /* Transaction state */
  z_ticket_t lock;                        /* Transaction internal lock */
};

struct raleighsl_object {
  z_cache_entry_t cache_entry;            /* Object Cache Entry */
  z_dlink_node_t journal;                 /* Object Journal Node */

  z_task_rwcsem_t rwcsem;                 /* Object RWC-Task-Lock */
  uint64_t pending_txn_id;                /* Pending Transaction Id */

  const raleighsl_object_plug_t *plug;    /* Object plugin */

  int requires_balancing; /* TODO: REMOVE ME! */

  void *devbufs;                          /* Object Device buffers */
  void *membufs;                          /* Object Memory buffers */
};

struct raleighsl_semantic {
  z_task_rwcsem_t rwcsem;                 /* Semantic RWC-Task-Lock */

  const raleighsl_semantic_plug_t *plug;  /* Semantic plugin */

  raleighsl_object_t *root;

  uint64_t next_oid;                      /* Next Object-ID */
};

struct raleighsl_journal {
  z_spinlock_t   lock;                    /* Object list lock */
  z_dlink_node_t objects;                 /* Dirty Objects */
  uint64_t       otime;                   /* Oldest entry Time */
};

struct raleighsl_blkcache {
  z_cache_t *cache;
};

struct raleighsl_device {
  const raleighsl_device_plug_t *plug;    /* Device plugin */
};

struct raleighsl {
  #define __RALEIGHSL_DECLARE_PLUG(name)                \
    struct {                                            \
      const raleighsl_ ## name ## _plug_t *plug;        \
      z_opaque_t data;                                  \
    } name;

  /* File-system plugins */
  __RALEIGHSL_DECLARE_PLUG(format)        /* Disk Format plug */
  __RALEIGHSL_DECLARE_PLUG(space)         /* Space Allocator */

  #undef __RALEIGHSL_DECLARE_PLUG

  raleighsl_semantic_t  semantic;         /* Semantic Layer */
  raleighsl_txn_mgr_t * txn_mgr;          /* Transaction Manager */
  raleighsl_journal_t   journal;          /* Journal Layer */

  z_cache_t *           obj_cache;
  raleighsl_device_t *  device;
  z_hash_map_t          plugins;
  raleighsl_master_t    master;
};

__Z_END_DECLS__

#endif /* !_RALEIGHSL_TYPES_H_ */