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

#ifndef __MEMCACHE_PRIVATE_H__
#define __MEMCACHE_PRIVATE_H__

#include <zcl/hashtable.h>
#include <zcl/skiplist.h>
#include <zcl/tree.h>

#define MEMCACHE_USE_TREE                         0
#define MEMCACHE_USE_SKIP_LIST                    0
#define MEMCACHE_USE_HASH_TABLE                   1

#define MEMCACHE_OBJECT(x)                        Z_CAST(memcache_object_t, x)
#define MEMCACHE(x)                               Z_CAST(memcache_t, x)

#define MEMCACHE_OBJECT_NUMBER                    (1)

typedef struct memcache_object {
    uint32_t key_size;
    uint32_t value_size;
    uint32_t iflags;
    uint32_t flags;
    uint32_t exptime;
    uint64_t cas;

    union {
        uint8_t *blob;
        uint64_t number;
    } data;

    uint8_t key[1];
} memcache_object_t;

typedef struct memcache {
    z_memory_t *    memory;
#if MEMCACHE_USE_TREE
    z_tree_t        table;
#elif MEMCACHE_USE_SKIP_LIST
    z_skip_list_t   table;
#elif MEMCACHE_USE_HASH_TABLE
    z_hash_table_t  table;
#endif
} memcache_t;

memcache_t *        memcache_alloc            (z_memory_t *memory);
void                memcache_free             (memcache_t *memcache);
void                memcache_clear            (memcache_t *memcache);
int                 memcache_insert           (memcache_t *memcache,
                                               memcache_object_t *item);
int                 memcache_remove           (memcache_t *memcache,
                                               memcache_object_t *item);
memcache_object_t * memcache_lookup           (memcache_t *memcache,
                                               const z_slice_t *key);


memcache_object_t * memcache_object_alloc     (z_memory_t *memory,
                                               uint32_t key_size);
void                memcache_object_free      (z_memory_t *memory,
                                               memcache_object_t *object);
int                 memcache_object_set       (memcache_object_t *object,
                                               z_memory_t *memory,
                                               const z_slice_t *value);
int                 memcache_object_prepend   (memcache_object_t *object,
                                               z_memory_t *memory,
                                               const z_slice_t *value);
int                 memcache_object_append    (memcache_object_t *object,
                                               z_memory_t *memory,
                                               const z_slice_t *value);

#endif /* !__MEMCACHE_PRIVATE_H__ */

