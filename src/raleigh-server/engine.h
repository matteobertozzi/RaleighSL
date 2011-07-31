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

#ifndef _STORAGE_ENGINE_H_
#define _STORAGE_ENGINE_H_

#include <zcl/messageq.h>

#include <zcl/hashtable.h>
#include <zcl/skiplist.h>
#include <zcl/tree.h>

#define USE_TREE                            0
#define USE_SKIP_LIST                       0
#define USE_HASH_TABLE                      1

typedef enum storage_code {
    STORAGE_ERROR,
    STORAGE_MESSAGE,

    /* Memcache Commands */
    STORAGE_MEMCACHE_GET,
    STORAGE_MEMCACHE_BGET,
    STORAGE_MEMCACHE_ADD,
    STORAGE_MEMCACHE_SET,
    STORAGE_MEMCACHE_REPLACE,
    STORAGE_MEMCACHE_PREPEND,
    STORAGE_MEMCACHE_APPEND,
    STORAGE_MEMCACHE_CAS,
    STORAGE_MEMCACHE_INCR,
    STORAGE_MEMCACHE_DECR,
    STORAGE_MEMCACHE_GETS,
    STORAGE_MEMCACHE_DELETE,
    STORAGE_MEMCACHE_FLUSH_ALL,
} storage_code_t;

typedef enum storage_state {
    STORAGE_STATE_OK,
    STORAGE_STATE_ERROR,
    STORAGE_STATE_NO_MEMORY,
    STORAGE_STATE_STORED,
    STORAGE_STATE_NOT_STORED,
    STORAGE_STATE_DELETED,
    STORAGE_STATE_EXISTS,
    STORAGE_STATE_NOT_EXISTS,

    STORAGE_STATE_MEMCACHE_NUMBER,
    STORAGE_STATE_MEMCACHE_NOT_NUMBER,
    STORAGE_STATE_MEMCACHE_BLOB,
} storage_state_t;

struct storage {
    z_memory_t *    memory;
#if USE_TREE
    z_tree_t        table;
#elif USE_SKIP_LIST
    z_skip_list_t   table;
#elif USE_HASH_TABLE
    z_hash_table_t  table;
#endif
};

int         storage_alloc        (struct storage *storage,
                                  z_memory_t *memory);
void        storage_free         (struct storage *storage);

int         storage_request_exec (struct storage *storage,
                                  z_message_t *msg);

#endif /* !_STORAGE_ENGINE_H_ */

