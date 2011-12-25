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

#ifndef _RALEIGHFS_TYPES_H_
#define _RALEIGHFS_TYPES_H_

#include <raleighfs/plugins.h>

#include <zcl/hashtable.h>
#include <zcl/thread.h>
#include <zcl/object.h>

#define RALEIGHFS_MASTER_MAGIC          ("R4l3igHfS-v5")
#define RALEIGHFS_MASTER_QMAGIC         (0xf5ba5028cb6afc76UL)

struct raleighfs_rwlock {
    z_spinlock_t lock;
    int access_count;                   /* -1 writer in, > 0 readers in */
    int wr_waiting;                     /* Writer waiting */
};

struct raleighfs_key {
    uint64_t body[4];
};

struct raleighfs_master {
    uint8_t  mb_magic[12];              /* Master block magic */

    uint64_t mb_ctime;                  /* File-system creation time */
    uint32_t mb_format;                 /* File-system format in use */

    uint8_t  mb_uuid[16];               /* File-system 128-bit uuid in use */
    uint8_t  mb_label[16];              /* Files-ystem label in use */

    uint64_t mb_qmagic;                 /* Master block end magic */
} __attribute__((__packed__));

struct raleighfs {
    Z_OBJECT_TYPE

    #define __RALEIGHFS_DECLARE_PLUG(name)                                  \
        struct {                                                            \
            raleighfs_ ## name ## _plug_t *plug;                            \
            raleighfs_plug_data_t data;                                     \
        } name;

    /* File-system plugins */
    __RALEIGHFS_DECLARE_PLUG(objcache)             /* In-memory Object cache */
    __RALEIGHFS_DECLARE_PLUG(semantic)             /* Semantic Layer */
    __RALEIGHFS_DECLARE_PLUG(format)               /* Disk Format plug */
    __RALEIGHFS_DECLARE_PLUG(space)                /* Space Allocator */
    __RALEIGHFS_DECLARE_PLUG(key)                  /* Key Layer */
    /* OID Allocator */
    /* Journal Layer */

    #undef __RALEIGHFS_PLUG

    raleighfs_device_t *device;                    /* File-system device */
    raleighfs_master_t  master;                    /* Master super block */
    z_hash_table_t      plugins;                   /* Loaded plugs table */

    raleighfs_rwlock_t  lock;                      /* Semantic ops-lock */
};

struct raleighfs_objdata {
    raleighfs_object_plug_t *plug;
    raleighfs_key_t     key;

    void *              devbufs;
    void *              membufs;
    raleighfs_rwlock_t  lock;
    unsigned int        refs;
};

struct raleighfs_object {
    raleighfs_objdata_t *internal;
};

struct raleighfs_device {
    raleighfs_device_plug_t *plug;
    raleighfs_plug_data_t data;
};

#endif /* !_RALEIGHFS_TYPES_H_ */

