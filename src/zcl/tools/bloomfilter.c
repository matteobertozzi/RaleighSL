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

#include <zcl/bloomfilter.h>
#include <zcl/memset.h>

#define __bit_mask(n)                       (1 << ((n)  & 0x7))

#define __bit_byte(filter, nblock, n)                                       \
    (*((filter)->map + (nblock) + ((n) >> 3)))

#define __bit_set(filter, nblock, n)                                        \
    __bit_byte(filter, nblock, n) |= __bit_mask(n)

#define __bit_test(filter, nblock, n)                                       \
    (__bit_byte(filter, nblock, n) & __bit_mask(n))

z_bloom_filter_t *z_bloom_filter_alloc (z_bloom_filter_t *filter,
                                        z_memory_t *memory,
                                        z_bloom_hash_t hash_func,
                                        unsigned int capacity,
                                        unsigned int num_slices,
                                        unsigned int bits_per_slice,
                                        void *user_data)
{
    unsigned int size;

    if ((filter = z_object_alloc(memory, filter, z_bloom_filter_t)) == NULL)
        return(NULL);

    bits_per_slice = z_align_up(bits_per_slice, 8);
    size = num_slices * (bits_per_slice >> 3);
    if (!(filter->map = z_object_block_zalloc(filter, unsigned char, size))) {
        z_object_free(filter);
        return(NULL);
    }

    filter->hash_func = hash_func;
    filter->capacity = capacity;
    filter->num_slices = num_slices;
    filter->bits_per_slice = bits_per_slice;
    filter->user_data = user_data;

    return(filter);
}

void z_bloom_filter_free (z_bloom_filter_t *filter) {
    z_object_block_free(filter, filter->map);
    z_object_free(filter);
}

void z_bloom_filter_clear (z_bloom_filter_t *filter) {
    unsigned int size;

    size = filter->num_slices * (filter->bits_per_slice >> 3);
    z_memzero(filter->map, size);
}

int z_bloom_filter_add (z_bloom_filter_t *filter, const void *key) {
    unsigned int hashes[Z_BLOOM_FILTER_MAX_SLICES];
    unsigned int offset;
    unsigned int shift;
    unsigned int mask;
    unsigned int hi;

    if (filter->hash_func(filter->user_data, key, hashes, filter->num_slices))
        return(-1);

    offset = 0U;
    shift = filter->bits_per_slice >> 3;
    mask = filter->bits_per_slice - 1;
    hi = filter->num_slices;
    while (hi--) {
        __bit_set(filter, offset, hashes[hi] & mask);
        offset += shift;
    }

    return(0);
}

int z_bloom_filter_contains (z_bloom_filter_t *filter, const void *key) {
    unsigned int hashes[Z_BLOOM_FILTER_MAX_SLICES];
    unsigned int offset;
    unsigned int shift;
    unsigned int mask;
    unsigned int hi;

    if (filter->hash_func(filter->user_data, key, hashes, filter->num_slices))
        return(-1);

    offset = 0U;
    shift = filter->bits_per_slice >> 3;
    mask = filter->bits_per_slice - 1;
    hi = filter->num_slices;
    while (hi--) {
        if (!__bit_test(filter, offset, hashes[hi] & mask))
            return(0);

        offset += shift;
    }

    return(1);
}

