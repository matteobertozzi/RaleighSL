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

#ifndef _Z_BLOOM_FILTER_H_
#define _Z_BLOOM_FILTER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/types.h>

#ifndef Z_BLOOM_FILTER_MAX_SLICES
    #define Z_BLOOM_FILTER_MAX_SLICES          (8)
#endif /* !Z_BLOOM_MAX_SLICES */

Z_TYPEDEF_STRUCT(z_bloom_filter)

typedef int (*z_bloom_hash_t)   (void *user_data,
                                 const void *key,
                                 unsigned int *hashes,
                                 unsigned int nhash);

struct z_bloom_filter {
    Z_OBJECT_TYPE

    z_bloom_hash_t  hash_func;
    void *          user_data;
    unsigned char * map;

    unsigned int    bits_per_slice;
    unsigned int    num_slices;
    unsigned int    capacity;
    unsigned int    count;
};

z_bloom_filter_t *  z_bloom_filter_alloc        (z_bloom_filter_t *filter,
                                                 z_memory_t *memory,
                                                 z_bloom_hash_t hash_func,
                                                 unsigned int capacity,
                                                 unsigned int num_slices,
                                                 unsigned int bits_per_slice,
                                                 void *user_data);
void                z_bloom_filter_free         (z_bloom_filter_t *filter);

void                z_bloom_filter_clear        (z_bloom_filter_t *filter);

int                 z_bloom_filter_add          (z_bloom_filter_t *filter,
                                                 const void *key);

int                 z_bloom_filter_contains     (z_bloom_filter_t *filter,
                                                 const void *key);

__Z_END_DECLS__

#endif /* !_Z_BLOOM_FILTER_H_ */

