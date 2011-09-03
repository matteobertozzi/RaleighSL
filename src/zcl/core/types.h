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

#ifndef _Z_TYPES_H_
#define _Z_TYPES_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

/* Objects */
#define z_hash_type_id                     (0x1010)

#define z_iopoll_type_id                   (0x2010)
#define z_socket_poll_type_id              (z_iopoll_type_id | 0x1)

#define z_work_queue_type_id               (0x3010)
#define z_messageq_type_id                 (0x3020)

#define z_tree_type_id                     (0x5010)
#define z_stree_type_id                    (0x5020)
#define z_trie_type_id                     (0x5030)
#define z_queue_type_id                    (0x5040)
#define z_deque_type_id                    (0x5050)
#define z_skip_list_type_id                (0x5060)
#define z_hash_table_type_id               (0x5070)
#define z_bloom_filter_type_id             (0x5080)
#define z_buffer_type_id                   (0x5090)
#define z_chunkq_type_id                   (0x50A0)
#define z_cache_type_id                    (0x50B0)
#define z_space_map_type_id                (z_tree_type_id | 0x1)
#define z_extent_tree_type_id              (z_tree_type_id | 0x2)

Z_TYPEDEF_UNION(z_data)
#define Z_DATA(x)                          Z_CAST(z_data_t, x)
#define Z_DATA_PTR(x)                      (Z_DATA(x)->ptr)
#define Z_DATA_FD(x)                       (Z_DATA(x)->fd)
#define Z_DATA_U32(x)                      (Z_DATA(x)->u32)
#define Z_DATA_U64(x)                      (Z_DATA(x)->u64)

union z_data {
    void *   ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
};

typedef int          (*z_compare_t)         (void *user_data,
                                             const void *a,
                                             const void *b);

typedef unsigned int (*z_object_hash_t)     (void *user_data,
                                             const void *data);

typedef int          (*z_kvforeach_t)       (void *user_data,
                                             const void *key,
                                             void *value);
typedef int          (*z_foreach_t)         (void *user_data,
                                             void *object);

typedef void         (*z_invoker_t)         (void *user_data);

/* Dynamic Memory & Objects */
typedef unsigned int (*z_resize_policy_t)   (void *user_data,
                                             unsigned int used,
                                             unsigned int size);
/* Input/Output */
typedef int          (*z_iowrite_t)         (void *user_data,
                                             int fd);
typedef int          (*z_ioread_t)          (void *user_data,
                                             int fd);

typedef unsigned int (*z_iofetch_t)         (void *user_data,
                                             void *buffer,
                                             unsigned int n);
typedef unsigned int (*z_iopush_t)          (void *user_data,
                                             const void *buffer,
                                             unsigned int n);

__Z_END_DECLS__

#endif /* !_Z_TYPES_H_ */

