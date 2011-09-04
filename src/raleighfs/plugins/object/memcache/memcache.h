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

#ifndef _RALEIGHFS_OBJECT_MEMCACHE_H_
#define _RALEIGHFS_OBJECT_MEMCACHE_H_

#include <raleighfs/execute.h>
#include <raleighfs/plugins.h>

typedef enum raleighfs_memcache_state {
    RALEIGHFS_MEMCACHE_OK,
    RALEIGHFS_MEMCACHE_STORED,
    RALEIGHFS_MEMCACHE_DELETED,
    RALEIGHFS_MEMCACHE_NOT_FOUND,
    RALEIGHFS_MEMCACHE_VERSION,
    RALEIGHFS_MEMCACHE_ERROR_GENERIC,
    RALEIGHFS_MEMCACHE_BAD_CMDLINE,
    RALEIGHFS_MEMCACHE_OBJ_TOO_LARGE,
    RALEIGHFS_MEMCACHE_NO_MEMORY,
    RALEIGHFS_MEMCACHE_INVALID_DELTA,
    RALEIGHFS_MEMCACHE_BLOB,
    RALEIGHFS_MEMCACHE_NUMBER,
    RALEIGHFS_MEMCACHE_NOT_NUMBER,
    RALEIGHFS_MEMCACHE_NOT_STORED,
    RALEIGHFS_MEMCACHE_EXISTS,
} raleighfs_memcache_state_t;

typedef enum raleighfs_memcache_code {
    /* Memcache Commands */
    RALEIGHFS_MEMCACHE_GET          = RALEIGHFS_QUERY  | 1,
    RALEIGHFS_MEMCACHE_BGET         = RALEIGHFS_QUERY  | 2,
    RALEIGHFS_MEMCACHE_ADD          = RALEIGHFS_INSERT | 3,
    RALEIGHFS_MEMCACHE_SET          = RALEIGHFS_INSERT | 4,
    RALEIGHFS_MEMCACHE_REPLACE      = RALEIGHFS_INSERT | 5,
    RALEIGHFS_MEMCACHE_PREPEND      = RALEIGHFS_INSERT | 6,
    RALEIGHFS_MEMCACHE_APPEND       = RALEIGHFS_INSERT | 7,
    RALEIGHFS_MEMCACHE_CAS          = RALEIGHFS_INSERT | 8,
    RALEIGHFS_MEMCACHE_INCR         = RALEIGHFS_UPDATE | 9,
    RALEIGHFS_MEMCACHE_DECR         = RALEIGHFS_UPDATE | 10,
    RALEIGHFS_MEMCACHE_GETS         = RALEIGHFS_QUERY  | 11,
    RALEIGHFS_MEMCACHE_DELETE       = RALEIGHFS_REMOVE | 12,
    RALEIGHFS_MEMCACHE_FLUSH_ALL    = RALEIGHFS_REMOVE | 13,
} raleighfs_memcache_code_t;

extern const uint8_t RALEIGHFS_OBJECT_MEMCACHE_UUID[16];
extern raleighfs_object_plug_t raleighfs_object_memcache;

#endif /* !_RALEIGHFS_OBJECT_MEMCACHE_H_ */

