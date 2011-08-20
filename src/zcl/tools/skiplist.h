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

#ifndef _Z_SKIP_LIST_H_
#define _Z_SKIP_LIST_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/types.h>

#ifndef Z_SKIP_LIST_MAX_HEIGHT
    #define Z_SKIP_LIST_MAX_HEIGHT          (16)
#endif /* !Z_SKIP_LIST_MAX_HEIGHT */

#define Z_SKIP_LIST(x)                      Z_CAST(z_skip_list_t, x)

Z_TYPEDEF_STRUCT(z_skip_node)
Z_TYPEDEF_STRUCT(z_skip_list)
Z_TYPEDEF_STRUCT(z_skip_iter)

struct z_skip_node {
    void *              data;
    z_skip_node_t *     next[1];
};

struct z_skip_list {
    Z_OBJECT_TYPE

    z_skip_node_t *     head;

    z_compare_t         key_compare;
    z_mem_free_t        data_free;
    void *              user_data;

    unsigned int        levels;
    unsigned int        seed;
};

struct z_skip_iter {
    const z_skip_list_t *   skip;
    z_skip_node_t *         current;
};

/* Skip-List methods */
z_skip_list_t *     z_skip_list_alloc           (z_skip_list_t *skip,
                                                 z_memory_t *memory,
                                                 z_compare_t key_compare,
                                                 z_mem_free_t data_free,
                                                 void *user_data);
void                z_skip_list_free            (z_skip_list_t *skip);

int                 z_skip_list_insert          (z_skip_list_t *skip,
                                                 void *data);
int                 z_skip_list_remove          (z_skip_list_t *skip,
                                                 const void *key);
void                z_skip_list_clear           (z_skip_list_t *skip);

void *              z_skip_list_lookup          (const z_skip_list_t *skip,
                                                 const void *key);
void *              z_skip_list_lookup_custom   (const z_skip_list_t *skip,
                                                 z_compare_t key_compare,
                                                 const void *key);

void *              z_skip_list_lookup_min      (const z_skip_list_t *skip);
void *              z_skip_list_lookup_max      (const z_skip_list_t *skip);

/* Skip-List Iterator methods */
int                 z_skip_iter_init            (z_skip_iter_t *iter,
                                                 const z_skip_list_t *skip);
void *              z_skip_iter_next            (z_skip_iter_t *iter);
void *              z_skip_iter_prev            (z_skip_iter_t *iter);

void *              z_skip_iter_lookup          (z_skip_iter_t *iter,
                                                 const void *key);
void *              z_skip_iter_lookup_custom   (z_skip_iter_t *iter,
                                                 z_compare_t key_compare,
                                                 const void *key);

void *              z_skip_iter_lookup_min      (z_skip_iter_t *iter);
void *              z_skip_iter_lookup_max      (z_skip_iter_t *iter);

__Z_END_DECLS__

#endif /* _Z_SKIP_LIST_H_ */

