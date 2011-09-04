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

#ifndef _Z_CACHE_H_
#define _Z_CACHE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/types.h>

#define Z_CACHE(x)                      Z_CAST(z_cache_t, x)

Z_TYPEDEF_STRUCT(z_cache_node)
Z_TYPEDEF_STRUCT(z_cache)

struct z_cache_node {
    z_cache_node_t *next;
    z_cache_node_t *prev;
    z_cache_node_t *hash;
    void *          data;
};

struct z_cache {
    Z_OBJECT_TYPE

    z_cache_node_t **table;
    z_cache_node_t * head;

    z_mem_free_t     free_func;
    z_object_hash_t  hash_func;
    z_compare_t      cmp_func;
    void *           user_data;

    unsigned int     used;
    unsigned int     size;
};

z_cache_t *     z_cache_alloc           (z_cache_t *cache,
                                         z_memory_t *memory,
                                         unsigned int size,
                                         z_object_hash_t hash_func,
                                         z_compare_t cmp_func,
                                         z_mem_free_t free_func,
                                         void *user_data);
void            z_cache_free            (z_cache_t *cache);

int             z_cache_add             (z_cache_t *cache,
                                         void *data);
int             z_cache_remove          (z_cache_t *cache,
                                         const void *key);

void *          z_cache_lookup          (z_cache_t *cache,
                                         const void *key);
void *          z_cache_lookup_custom   (z_cache_t *cache,
                                         z_object_hash_t hash_func,
                                         z_compare_t cmp_func,
                                         const void *key);

__Z_END_DECLS__

#endif /* !_Z_CACHE_H_ */

