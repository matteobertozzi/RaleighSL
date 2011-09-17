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

#include <raleighfs/object.h>

#include <zcl/memory.h>
#include <zcl/stream.h>
#include <zcl/memcpy.h>
#include <zcl/memcmp.h>
#include <zcl/strtol.h>
#include <zcl/hash.h>

#include <stdio.h>

#include "memcache_p.h"
#include "memcache.h"

/* ============================================================================
 *  Memcache Table
 */
#define __HASH_FUNC_SEED                (0)
#define __HASH_FUNC                     z_hash32_murmur3

#if MEMCACHE_USE_HASH_TABLE
static unsigned int __object_hash (void *user_data,
                                   const void *data)
{
    memcache_object_t *item = MEMCACHE_OBJECT(data);
    return(__HASH_FUNC(item->key, item->key_size, __HASH_FUNC_SEED));
}
#endif /* MEMCACHE_USE_HASH_TABLE */

static int __object_compare (void *user_data,
                             const void *obj1,
                             const void *obj2)
{
    memcache_object_t *a = MEMCACHE_OBJECT(obj1);
    memcache_object_t *b = MEMCACHE_OBJECT(obj2);
    unsigned int min_len;
    int cmp;

    min_len = z_min(a->key_size, b->key_size);
    if ((cmp = z_memcmp(a->key, b->key, min_len)))
        return(cmp);

    return(a->key_size - b->key_size);
}

#if MEMCACHE_USE_HASH_TABLE
static unsigned int __hash (void *user_data, const void *buf, unsigned int n) {
    z_hash32_update(Z_HASH32(user_data), buf, n);
    return(n);
}

static unsigned int __object_key_hash (void *user_data,
                                       const void *data)
{
    const z_stream_extent_t *key = (const z_stream_extent_t *)data;
    z_hash32_t hash;
    z_hash32_init(&hash, __HASH_FUNC, __HASH_FUNC_SEED);
    z_stream_fetch(key->stream, key->offset, key->length, __hash, &hash);
    return(z_hash32_digest(&hash));
}
#endif /* MEMCACHE_USE_HASH_TABLE */

static int __object_key_compare (void *user_data,
                                 const void *obj1,
                                 const void *obj2)
{
    memcache_object_t *a = MEMCACHE_OBJECT(obj1);
    z_stream_extent_t *b = (z_stream_extent_t *)obj2;
    unsigned int min_len;
    int cmp;

    min_len = z_min(a->key_size, b->length);
    if ((cmp = z_stream_memcmp(b->stream, b->offset, a->key, min_len)))
        return(-cmp);

    return(a->key_size - b->length);
}

memcache_t *memcache_alloc (z_memory_t *memory) {
    memcache_t *memcache;
    void *table;

    if ((memcache = z_memory_struct_alloc(memory, memcache_t)) == NULL)
        return(NULL);

    memcache->memory = memory;

#if MEMCACHE_USE_TREE
    table = z_tree_alloc(&(memcache->table), memory,
                         &z_tree_avl, __object_compare,
                         (z_mem_free_t)memcache_object_free,
                         memory);
#elif MEMCACHE_USE_SKIP_LIST
    table = z_skip_list_alloc(&(memcache->table), memory,
                              __object_compare,
                              (z_mem_free_t)memcache_object_free,
                              memory);
#else
    table = z_hash_table_alloc(&(memcache->table), memory,
                               &z_hash_table_chain, 8192,
                               __object_hash, __object_compare,
                               z_hash_table_grow_policy, NULL,
                               (z_mem_free_t)memcache_object_free,
                               memory);
#endif

    if (table == NULL) {
        z_memory_struct_free(memory, memcache_t, memcache);
        return(NULL);
    }

    return(memcache);
}

void memcache_free (memcache_t *memcache) {
#if MEMCACHE_USE_TREE
    z_tree_free(&(memcache->table));
#elif MEMCACHE_USE_SKIP_LIST
    z_skip_list_free(&(memcache->table));
#elif MEMCACHE_USE_HASH_TABLE
    z_hash_table_free(&(memcache->table));
#endif
}

void memcache_clear (memcache_t *memcache) {
#if MEMCACHE_USE_TREE
    z_tree_clear(&(memcache->table));
#elif MEMCACHE_USE_SKIP_LIST
    z_skip_list_clear(&(memcache->table));
#else
    z_hash_table_clear(&(memcache->table));
#endif
}

int memcache_insert (memcache_t *memcache, memcache_object_t *item) {
#if MEMCACHE_USE_TREE
    return(z_tree_insert(&(memcache->table), item));
#elif MEMCACHE_USE_SKIP_LIST
    return(z_skip_list_insert(&(memcache->table), item));
#else
    return(z_hash_table_insert(&(memcache->table), item));
#endif
}

int memcache_remove (memcache_t *memcache, memcache_object_t *item) {
#if MEMCACHE_USE_TREE
    return(z_tree_remove(&(memcache->table), item));
#elif MEMCACHE_USE_SKIP_LIST
    return(z_skip_list_remove(&(memcache->table), item));
#else
    return(z_hash_table_remove(&(memcache->table), item));
#endif
}

memcache_object_t *memcache_lookup (memcache_t *memcache,
                                    const z_stream_extent_t *key)
{
    void *item;

#if MEMCACHE_USE_TREE
    item = z_tree_lookup_custom(&(memcache->table),
                                __object_key_compare,
                                key);
#elif MEMCACHE_USE_SKIP_LIST
    item = z_skip_list_lookup_custom(&(memcache->table),
                                     __object_key_compare,
                                     key);
#else
    item = z_hash_table_lookup_custom(&(memcache->table),
                                      __object_key_hash,
                                      __object_key_compare,
                                      key);
#endif

    return(MEMCACHE_OBJECT(item));
}

/* ============================================================================
 *  Memcache Object
 */
memcache_object_t *memcache_object_alloc (z_memory_t *memory,
                                          uint32_t key_size)
{
    memcache_object_t *obj;
    unsigned int size;

    /* TODO: Use a custom allocator with pool */

    size = sizeof(memcache_object_t) + (key_size - 1);
    if ((obj = z_memory_block_alloc(memory, memcache_object_t, size)) != NULL) {
        obj->key_size = key_size;
        obj->value_size = 0;
        obj->iflags = 0;
        obj->flags = 0;
        obj->exptime = 0;
        obj->cas = 0;
        obj->data.blob = NULL;
    }

    return(obj);
}

void memcache_object_free (z_memory_t *memory,
                           memcache_object_t *object)
{
    if (!(object->iflags & MEMCACHE_OBJECT_NUMBER)) {
        if (object->data.blob != NULL)
            z_memory_blob_free(memory, object->data.blob);
    }

    z_memory_block_free(memory, object);
}


int memcache_object_set (memcache_object_t *object,
                         z_memory_t *memory,
                         const z_stream_extent_t *value)
{
    char vbuffer[16];
    uint8_t *blob;

    if (value->length < 16) {
        z_stream_read_extent(value, vbuffer);
        vbuffer[value->length] = '\0';

        if (object->iflags & MEMCACHE_OBJECT_NUMBER)
            blob = NULL;
        else
            blob = object->data.blob;

        if (z_strtou64(vbuffer, 10, &(object->data.number))) {
            if (blob != NULL)
                 z_memory_blob_free(memory, blob);

            object->iflags |= MEMCACHE_OBJECT_NUMBER;
            return(0);
        }
    }

    /* set data to blob */
    if (object->iflags & MEMCACHE_OBJECT_NUMBER)
        blob = z_memory_blob_alloc(memory, value->length);
    else
        blob = z_memory_blob_realloc(memory, object->data.blob, value->length);

    if (blob == NULL)
        return(1);

    object->data.blob = blob;
    object->value_size = value->length;
    object->iflags &= ~MEMCACHE_OBJECT_NUMBER;

    if (value->length < 16)
        z_memcpy(blob, vbuffer, value->length);
    else
        z_stream_read_extent(value, blob);

    return(0);
}

int memcache_object_prepend (memcache_object_t *object,
                             z_memory_t *memory,
                             const z_stream_extent_t *value)
{
    char vbuffer[16];
    char number[16];
    unsigned int n;
    uint8_t *blob;

    if (object->iflags & MEMCACHE_OBJECT_NUMBER) {
        if (value->length < 16) {
            uint64_t number;

            z_stream_read_extent(value, vbuffer);
            if (z_strtou64(vbuffer, 10, &number)) {
                unsigned int size = value->length;
                while (size--)
                    object->data.number *= 10;
                object->data.number += number;
                return(0);
            }
        }

        n = z_u64tostr(object->data.number, number, sizeof(number), 10);
        blob = z_memory_blob_alloc(memory, n + value->length);
    } else {
        n = 0;
        blob = z_memory_blob_realloc(memory,
                                     object->data.blob,
                                     object->value_size + value->length);
    }

    if (blob == NULL)
        return(1);

    /* Set This is not a number */
    object->iflags &= ~MEMCACHE_OBJECT_NUMBER;

    /* Copy number to blob if any */
    z_memcpy(blob, number, n);

    /* Preprend data to blob */
    z_memmove(blob + value->length, blob, object->value_size);
    object->value_size += value->length;
    object->data.blob = blob;

    if (n > 0 && value->length < 16)
        z_memcpy(blob, vbuffer, value->length);
    else
        z_stream_read_extent(value, blob);

    return(0);
}

int memcache_object_append (memcache_object_t *object,
                            z_memory_t *memory,
                            const z_stream_extent_t *value)
{
    char vbuffer[16];
    char number[16];
    unsigned int n;
    uint8_t *blob;

    if (object->iflags & MEMCACHE_OBJECT_NUMBER) {
        if (value->length < 16) {
            uint64_t number;

            z_stream_read_extent(value, vbuffer);
            if (z_strtou64(vbuffer, 10, &number)) {
                unsigned int size = value->length;
                while (size--)
                    object->data.number *= 10;
                object->data.number += number;
                return(0);
            }
        }

        n = z_u64tostr(object->data.number, number, sizeof(number), 10);
        blob = z_memory_blob_alloc(memory, n + value->length);
    } else {
        n = 0;
        blob = z_memory_blob_realloc(memory,
                                     object->data.blob,
                                     object->value_size + value->length);
    }

    if (blob == NULL)
        return(1);

    /* Set This is not a number */
    object->iflags &= ~MEMCACHE_OBJECT_NUMBER;

    /* Copy number to blob if any */
    z_memcpy(blob, number, n);

    /* Append data to blob */
    object->data.blob = blob;
    blob = blob + object->value_size;
    object->value_size += value->length;

    if (object->iflags & MEMCACHE_OBJECT_NUMBER && value->length < 16)
        z_memcpy(blob, vbuffer, value->length);
    else
        z_stream_read_extent(value, blob);

    return(0);
}

