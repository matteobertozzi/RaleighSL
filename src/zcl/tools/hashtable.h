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

#ifndef _Z_HASH_TABLE_H_
#define _Z_HASH_TABLE_H_

#include <zcl/object.h>
#include <zcl/types.h>

Z_TYPEDEF_CONST_STRUCT(z_hash_table_plug)
Z_TYPEDEF_STRUCT(z_hash_table)

#define Z_HASH_TABLE(x)                 Z_CAST(z_hash_table_t, x)

struct z_hash_table_plug {
    int     (*init)         (z_hash_table_t *table,
                             unsigned int size);
    void    (*uninit)       (z_hash_table_t *table);

    int     (*insert)       (z_hash_table_t *table,
                             void *data);
    int     (*remove)       (z_hash_table_t *table,
                             const void *key);
    void    (*clear)        (z_hash_table_t *table);

    int     (*resize)       (z_hash_table_t *table,
                             unsigned int new_size);

    void *  (*lookup)       (z_hash_table_t *table,
                             z_object_hash_t hash_func,
                             z_compare_t key_compare,
                             const void *key);

    void    (*foreach)      (z_hash_table_t *table,
                             z_foreach_t func,
                             void *user_data);
};

struct z_hash_table {
    Z_OBJECT_TYPE

    z_hash_table_plug_t *   plug;
    void *                  bucket;

    z_resize_policy_t       grow_policy;
    z_resize_policy_t       shrink_policy;
    z_object_hash_t         hash_func;
    z_compare_t             key_compare;
    z_mem_free_t            data_free;
    void *                  user_data;

    unsigned int            size;
    unsigned int            used;
};

extern z_hash_table_plug_t z_hash_table_open;
extern z_hash_table_plug_t z_hash_table_chain;

z_hash_table_t *  z_hash_table_alloc          (z_hash_table_t *table,
                                               z_memory_t *memory,
                                               z_hash_table_plug_t *plug,
                                               unsigned int size,
                                               z_object_hash_t hash_func,
                                               z_compare_t key_compare,
                                               z_resize_policy_t grow_policy,
                                               z_resize_policy_t shrink_policy,
                                               z_mem_free_t data_free,
                                               void *user_data);
void              z_hash_table_free           (z_hash_table_t *table);

int               z_hash_table_insert         (z_hash_table_t *table,
                                               void *data);
int               z_hash_table_remove         (z_hash_table_t *table,
                                               const void *key);
void              z_hash_table_clear          (z_hash_table_t *table);

void *            z_hash_table_lookup         (z_hash_table_t *table,
                                               const void *key);
void *            z_hash_table_lookup_custom  (z_hash_table_t *table,
                                               z_object_hash_t hash_func,
                                               z_compare_t key_compare,
                                               const void *key);

void              z_hash_table_foreach        (z_hash_table_t *table,
                                               z_foreach_t func,
                                               void *user_data);

/* Default Resize Policy */
unsigned int      z_hash_table_grow_policy    (void *user_data,
                                               unsigned int used,
                                               unsigned int size);
unsigned int      z_hash_table_shrink_policy  (void *user_data,
                                               unsigned int used,
                                               unsigned int size);

#endif /* _Z_HASH_TABLE_H_ */

