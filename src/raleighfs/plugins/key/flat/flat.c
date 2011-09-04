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

#include <raleighfs/types.h>

#include <zcl/memcpy.h>
#include <zcl/memset.h>
#include <zcl/memcmp.h>
#include <zcl/cache.h>
#include <zcl/hash.h>

#include "flat.h"

#define __KEY_CACHE_ITEM(x)         Z_CAST(struct key_cache_item, x)

#define __KEY_HASH32_SEED           (0)
#define __KEY_HASH32_FUNC           z_hash32_string

#define __KEY_CACHE_SIZE            (100)
#define __KEY_CACHE(fs)             Z_CACHE((fs)->key.data.ptr)

struct key_cache_item {
    uint64_t body[4];
    unsigned int name_length;
    unsigned char name[1];
};

static unsigned int __key_cache_hash (void *fs, const void *ptr) {
    struct key_cache_item *item = __KEY_CACHE_ITEM(ptr);
    return(__KEY_HASH32_FUNC(item->name, item->name_length, __KEY_HASH32_SEED));
}

static int __key_cache_compare (void *fs, const void *a, const void *b)  {
    struct key_cache_item *ia = __KEY_CACHE_ITEM(a);
    struct key_cache_item *ib = __KEY_CACHE_ITEM(b);
    unsigned int length;
    int cmp;

    length = z_min(ia->name_length, ib->name_length);
    if ((cmp = z_memcmp(ia->name, ib->name, length)))
        return(cmp);

    return(ia->name_length - ib->name_length);
}

static struct key_cache_item *__key_cache_item_alloc (raleighfs_t *fs,
                                                      raleighfs_key_t *key,
                                                      const z_rdata_t *name)
{
    struct key_cache_item *item;
    unsigned int name_length;
    unsigned int size;

    name_length = z_rdata_length(name);
    size = sizeof(struct key_cache_item) + name_length - 1;
    if ((item = z_object_block_alloc(fs, struct key_cache_item, size)) == NULL)
        return(NULL);

    /* Copy data to item cache */
    z_memcpy(item->body, key->body, 32);
    item->name_length = name_length;
    z_rdata_read(name, 0, item->name, name_length);

    return(item);
}

static void __key_cache_item_free (void *fs, void *ptr) {
    z_object_block_free(fs, ptr);
}

static unsigned int __name_hash_part (void *user_data,
                                      const void *buffer,
                                      unsigned int n)
{
    z_hash32_update(Z_HASH32(user_data), buffer, n);
    return(n);
}

static unsigned int __name_hash (void *fs, const void *ptr) {
    z_hash32_t hash;
    z_hash32_init(&hash, __KEY_HASH32_FUNC, __KEY_HASH32_SEED);
    z_rdata_fetch_all(Z_RDATA(ptr), __name_hash_part, &hash);
    return(z_hash32_digest(&hash));
}

static int __name_compare (void *fs, const void *a, const void *b) {
    struct key_cache_item *i = __KEY_CACHE_ITEM(a);
    z_rdata_t *name = Z_RDATA(b);
    unsigned int length;
    int cmp;

    length = z_rdata_length(name);
    if ((cmp = z_rdata_rawcmp(name, 0, i->name, z_min(length, i->name_length))))
        return(-cmp);

    return(i->name_length - length);
}

static raleighfs_errno_t __key_init (raleighfs_t *fs) {
    fs->key.data.ptr = z_cache_alloc(NULL, z_object_memory(fs),
                                     __KEY_CACHE_SIZE,
                                     __key_cache_hash,
                                     __key_cache_compare,
                                     __key_cache_item_free, fs);
    if (__KEY_CACHE(fs) == NULL)
        return(RALEIGHFS_ERRNO_NO_MEMORY);

    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __key_uninit (raleighfs_t *fs) {
    z_cache_free(__KEY_CACHE(fs));
    return(RALEIGHFS_ERRNO_NONE);
}

static int __key_compare (raleighfs_t *fs,
                          const raleighfs_key_t *a,
                          const raleighfs_key_t *b)
{
    return(z_memcmp(a, b, sizeof(raleighfs_key_t)));
}

static unsigned int __hash_name_part (void *user_data,
                                      const void *buffer,
                                      unsigned int n)
{
    z_hash_update(Z_HASH(user_data), buffer, n);
    return(n);
}

static void __key_hash (raleighfs_t *fs,
                        raleighfs_key_t *key,
                        const z_rdata_t *name)
{
    z_hash_t hash;

    z_hash_alloc(&hash, z_object_memory(fs), &z_hash160_plug_ripemd);
    z_rdata_fetch_all(name,  __hash_name_part, &hash);
    z_memzero(key->body, 32);
    z_hash_digest(&hash, key->body);
    z_hash_free(&hash);
}

static raleighfs_errno_t __key_object (raleighfs_t *fs,
                                       raleighfs_key_t *key,
                                       const z_rdata_t *name)
{
    struct key_cache_item *item;

    /* Try to lookup key from cache */
    item = __KEY_CACHE_ITEM(z_cache_lookup_custom(__KEY_CACHE(fs),
                                                  __name_hash,
                                                  __name_compare, name));
    if (item != NULL) {
        z_memcpy(key->body, item->body, 32);
        return(RALEIGHFS_ERRNO_NONE);
    }

    /* Key is not in cache, calculate on the fly */
    __key_hash(fs, key, name);

    /* Try to add item to cache */
    if ((item = __key_cache_item_alloc(fs, key, name)) != NULL) {
        if (z_cache_add(__KEY_CACHE(fs), item))
            __key_cache_item_free(fs, item);
    }

    return(RALEIGHFS_ERRNO_NONE);
}

const uint8_t RALEIGHFS_KEY_FLAT_UUID[16] = {
    197,1,250,214,220,224,66,250,148,11,182,108,221,165,190,195,
};

raleighfs_key_plug_t raleighfs_key_flat = {
    .info = {
        .type = RALEIGHFS_PLUG_TYPE_KEY,
        .uuid = RALEIGHFS_KEY_FLAT_UUID,
        .description = "Flat key",
        .label = "key-flat",
    },

    .init       = __key_init,
    .load       = __key_init,
    .unload     = __key_uninit,

    .compare    = __key_compare,

    .object     = __key_object,
};

