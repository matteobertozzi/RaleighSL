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

#include <zcl/chunkq.h>
#include <zcl/memcmp.h>
#include <zcl/hash.h>

#include <stdio.h>

#include "engine.h"

#define MEMCACHE_FLAGS_RESET                (0xffffff0000000000ULL)
#define MEMCACHE_ITEM(x)                    Z_CAST(memcache_object_t, x)
#define MEMCACHE_ITEM_NUMBER                ((uint64_t)1 << 40)
#define MEMCACHE_ITEM_CAS                   ((uint64_t)1 << 41)

/* ====================================================================== */
typedef struct memcache_object {
    uint32_t key_size;
    uint32_t value_size;
    uint64_t flags;
    uint32_t exptime;
    uint32_t cas;

    union {
        uint8_t *blob;
        uint64_t number;
    } data;

    uint8_t key[1];
} memcache_object_t;

static memcache_object_t *memcache_object_alloc (z_memory_t *memory,
                                                 uint32_t key_size)
{
    memcache_object_t *obj;
    unsigned int size;

    size = sizeof(memcache_object_t) + (key_size - 1);
    if ((obj = z_memory_block_alloc(memory, memcache_object_t, size)) != NULL) {
        obj->key_size = key_size;
        obj->value_size = 0;
        obj->flags = 0;
        obj->exptime = 0;
        obj->cas = 0;
        obj->data.blob = NULL;
    }

    return(obj);
}

static void memcache_object_free (z_memory_t *memory,
                                  memcache_object_t *object)
{
    if (!(object->flags & MEMCACHE_ITEM_NUMBER)) {
        if (object->data.blob != NULL)
            z_memory_blob_free(memory, object->data.blob);
    }
    z_memory_block_free(memory, object);
}


static int memcache_object_set (memcache_object_t *object,
                                z_memory_t *memory,
                                const z_stream_extent_t *value)
{
    char vbuffer[16];
    uint8_t *blob;

    if (value->length < 16) {
        z_stream_read_extent(value, vbuffer);
        vbuffer[value->length] = '\0';

        if (z_strtou64(vbuffer, 10, &(object->data.number))) {
            /* TODO: free blob */
            object->flags |= MEMCACHE_ITEM_NUMBER;
            return(0);
        }
    }

    /* set data to blob */
    if (object->flags & MEMCACHE_ITEM_NUMBER)
        blob = z_memory_blob_alloc(memory, value->length);
    else
        blob = z_memory_blob_realloc(memory, object->data.blob, value->length);

    if (blob == NULL)
        return(1);

    object->data.blob = blob;
    object->value_size = value->length;
    object->flags = object->flags & ~MEMCACHE_ITEM_NUMBER;

    if (value->length < 16)
        z_memcpy(blob, vbuffer, value->length);
    else
        z_stream_read_extent(value, blob);

    return(0);
}

static int memcache_object_prepend (memcache_object_t *object,
                                    z_memory_t *memory,
                                    const z_stream_extent_t *value)
{
    char vbuffer[16];
    char number[16];
    unsigned int n;
    uint8_t *blob;

    if (object->flags & MEMCACHE_ITEM_NUMBER) {
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
    object->flags = object->flags & ~MEMCACHE_ITEM_NUMBER;

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

static int memcache_object_append (memcache_object_t *object,
                                   z_memory_t *memory,
                                   const z_stream_extent_t *value)
{
    char vbuffer[16];
    char number[16];
    unsigned int n;
    uint8_t *blob;

    if (object->flags & MEMCACHE_ITEM_NUMBER) {
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
    object->flags = object->flags & ~MEMCACHE_ITEM_NUMBER;

    /* Copy number to blob if any */
    z_memcpy(blob, number, n);

    /* Append data to blob */
    object->data.blob = blob;
    blob = blob + object->value_size;
    object->value_size += value->length;

    if (object->flags & MEMCACHE_ITEM_NUMBER && value->length < 16)
        z_memcpy(blob, vbuffer, value->length);
    else
        z_stream_read_extent(value, blob);

    return(0);
}
/* ====================================================================== */

#if USE_HASH_TABLE
static unsigned int __item_hash (void *user_data,
                                 const void *data)
{
    memcache_object_t *item = MEMCACHE_ITEM(data);
    return(z_hash32_murmur2(item->key, item->key_size, 0));
    /* return(z_hash32_string(item->key, item->key_size, 0)); */
}
#endif
static int __item_compare (void *user_data,
                           const void *obj1,
                           const void *obj2)
{
    memcache_object_t *a = MEMCACHE_ITEM(obj1);
    memcache_object_t *b = MEMCACHE_ITEM(obj2);
    unsigned int min_len;
    int cmp;

    min_len = z_min(a->key_size, b->key_size);
    if ((cmp = z_memcmp(a->key, b->key, min_len)))
        return(cmp);

    return(a->key_size - b->key_size);
}
#if USE_HASH_TABLE
static unsigned int __hash (void *user_data, const void *buffer, unsigned int n) {
    unsigned int *hash = Z_UINT_PTR(user_data);
    *hash = z_hash32_murmur2(buffer, n, *hash);
    return(n);
}

static unsigned int __item_key_hash (void *user_data,
                                     const void *data)
{
    const z_stream_extent_t *key = (const z_stream_extent_t *)data;
    unsigned int hash = 0;
    z_stream_fetch(key->stream, key->offset, key->length, __hash, &hash);
    return(hash);
}
#endif

static int __item_key_compare (void *user_data,
                               const void *obj1,
                               const void *obj2)
{
    memcache_object_t *a = MEMCACHE_ITEM(obj1);
    z_stream_extent_t *b = (z_stream_extent_t *)obj2;
    unsigned int min_len;
    int cmp;

    min_len = z_min(a->key_size, b->length);
    if ((cmp = z_stream_memcmp(b->stream, b->offset, a->key, min_len)))
        return(-cmp);

    return(a->key_size - b->length);
}

static int __item_insert (struct storage *storage, memcache_object_t *item) {
#if USE_TREE
    return(z_tree_insert(&(storage->table), item));
#elif USE_SKIP_LIST
    return(z_skip_list_insert(&(storage->table), item));
#else
    return(z_hash_table_insert(&(storage->table), item));
#endif
}

static memcache_object_t *__item_lookup (struct storage *storage,
                                         const z_stream_extent_t *key)
{
    void *item;

#if USE_TREE
    item = z_tree_lookup_custom(&(storage->table), __item_key_compare, key);
#elif USE_SKIP_LIST
    item = z_skip_list_lookup_custom(&(storage->table), __item_key_compare, key);
#else
    item = z_hash_table_lookup_custom(&(storage->table), __item_key_hash, __item_key_compare, key);
#endif

    return(MEMCACHE_ITEM(item));
}

static int __item_remove (struct storage *storage, memcache_object_t *item) {
#if USE_TREE
    return(z_tree_remove(&(storage->table), item));
#elif USE_SKIP_LIST
    return(z_skip_list_remove(&(storage->table), item));
#else
    return(z_hash_table_remove(&(storage->table), item));
#endif
}

static void __items_clear (struct storage *storage) {
#if USE_TREE
    z_tree_clear(&(storage->table));
#elif USE_SKIP_LIST
    z_skip_list_clear(&(storage->table));
#else
    z_hash_table_clear(&(storage->table));
#endif
}

int storage_alloc (struct storage *storage, z_memory_t *memory) {
    void *table;

    storage->memory = memory;

#if USE_TREE
    table = z_tree_alloc(&(storage->table), memory,
                         &z_tree_avl, __item_compare,
                         (z_mem_free_t)memcache_object_free,
                         memory);
#elif USE_SKIP_LIST
    table = z_skip_list_alloc(&(storage->table), memory,
                              __item_compare,
                              (z_mem_free_t)memcache_object_free,
                              memory);
#elif USE_HASH_TABLE
    table = z_hash_table_alloc(&(storage->table), memory,
                               &z_hash_table_chain, 8192,
                               __item_hash, __item_compare,
                               z_hash_table_grow_policy, NULL,
                               (z_mem_free_t)memcache_object_free,
                               memory);
#endif

    return(table == NULL);
}

void storage_free (struct storage *storage) {
#if USE_TREE
    z_tree_free(&(storage->table));
#elif USE_SKIP_LIST
    z_skip_list_free(&(storage->table));
#elif USE_HASH_TABLE
    z_hash_table_free(&(storage->table));
#endif
}

static int __storage_get (struct storage *storage, z_message_t *msg) {
    memcache_object_t *item;
    z_stream_t req_stream;
    z_stream_t res_stream;
    z_stream_extent_t key;
    uint32_t klength;
    uint32_t count;

    z_message_response_stream(msg, &res_stream);
    z_message_request_stream(msg, &req_stream);
    z_stream_read_uint32(&req_stream, &count);

    while (count--) {
        z_stream_read_uint32(&req_stream, &klength);
        z_stream_set_extent(&req_stream, &key, req_stream.offset, klength);
        z_stream_seek(&req_stream, req_stream.offset + klength);

        if ((item = __item_lookup(storage, &key)) == NULL) {
            z_stream_write_uint32(&res_stream, STORAGE_STATE_NOT_EXISTS);
            continue;
        }

        if (item->flags & MEMCACHE_ITEM_NUMBER)
            z_stream_write_uint32(&res_stream, STORAGE_STATE_MEMCACHE_NUMBER);
        else
            z_stream_write_uint32(&res_stream, STORAGE_STATE_MEMCACHE_BLOB);

        z_stream_write_uint32(&res_stream, item->exptime);
        z_stream_write_uint32(&res_stream, item->flags & 0xffffffff);
        z_stream_write_uint64(&res_stream, item->cas);

        if (item->flags & MEMCACHE_ITEM_NUMBER) {
            z_stream_write_uint64(&res_stream, item->data.number);
        } else {
            z_stream_write_uint32(&res_stream, item->value_size);
            z_stream_write(&res_stream, item->data.blob, item->value_size);
        }
    }

    return(0);
}

static int __storage_set (struct storage *storage, z_message_t *msg) {
    memcache_object_t *item;
    z_stream_extent_t value;
    z_stream_extent_t key;
    z_stream_t stream;
    uint32_t klength;
    uint32_t vlength;
    uint32_t exptime;
    uint32_t flags;
    uint64_t cas;
    int msg_type;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(&stream, &klength);
    z_stream_read_uint32(&stream, &vlength);
    z_stream_read_uint32(&stream, &flags);
    z_stream_read_uint32(&stream, &exptime);
    z_stream_read_uint64(&stream, &cas);
    z_stream_set_extent(&stream, &key, stream.offset, klength);
    z_stream_set_extent(&stream, &value, stream.offset + klength, vlength);

    /* Do item lookup */
    item = __item_lookup(storage, &key);
    msg_type = z_message_type(msg);

    if (msg_type == STORAGE_MEMCACHE_ADD || msg_type == STORAGE_MEMCACHE_SET) {
        /*
         * SET: store this data,
         * ADD: but only if the server *doesn't* already hold data for this key.
         */
        if (item != NULL && msg_type == STORAGE_MEMCACHE_ADD) {
            z_message_set_state(msg, STORAGE_STATE_NOT_STORED);
            return(1);
        }

        /* Allocate new object */
        if (item == NULL) {
            if (!(item = memcache_object_alloc(storage->memory, key.length))) {
                z_message_set_state(msg, STORAGE_STATE_NO_MEMORY);
                return(2);
            }

            /* Set the key */
            z_stream_read_extent(&key, item->key);

            /* Add to table */
            __item_insert(storage, item);
        }

        /* Set Item Flags */
        item->exptime = exptime;
        item->flags = (item->flags & MEMCACHE_FLAGS_RESET) + flags;

        /* Read Item Data */
        memcache_object_set(item, storage->memory, &value);
    } else if (msg_type == STORAGE_MEMCACHE_CAS) {
        /* store this data but only if no one else has updated
         * since I last fetched it.
         */
        if (item == NULL) {
            z_message_set_state(msg, STORAGE_STATE_NOT_STORED);
            return(1);
        }

        if (item->cas != cas) {
            z_message_set_state(msg, STORAGE_STATE_EXISTS);
            return(2);
        }

        memcache_object_set(item, storage->memory, &value);
        item->cas++;
    } else if (msg_type == STORAGE_MEMCACHE_REPLACE) {
        /* store this data, but only if the server *does*
         * already hold data for this key.
         */
        if (item == NULL) {
            z_message_set_state(msg, STORAGE_STATE_NOT_STORED);
            return(1);
        }

        /* Set Item Flags */
        item->exptime = exptime;
        item->flags = (item->flags & MEMCACHE_FLAGS_RESET) + flags;

        /* Read Item Data */
        memcache_object_set(item, storage->memory, &value);
    } else if (msg_type == STORAGE_MEMCACHE_APPEND) {
        /* add this data to an existing key after existing data */
        if (item == NULL) {
            z_message_set_state(msg, STORAGE_STATE_NOT_STORED);
            return(1);
        }

        memcache_object_append(item, storage->memory, &value);
    } else if (msg_type == STORAGE_MEMCACHE_PREPEND) {
        /* add this data to an existing key before existing data */
        if (item == NULL) {
            z_message_set_state(msg, STORAGE_STATE_NOT_STORED);
            return(1);
        }

        memcache_object_prepend(item, storage->memory, &value);
    }

    z_message_set_state(msg, STORAGE_STATE_STORED);
    return(0);
}

static int __storage_arithmetic (struct storage *storage, z_message_t *msg) {
    memcache_object_t *item;
    z_stream_extent_t key;
    z_stream_t stream;
    uint32_t klength;
    uint64_t delta;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint64(&stream, &delta);
    z_stream_read_uint32(&stream, &klength);
    z_stream_set_extent(&stream, &key, stream.offset, klength);

    if ((item = __item_lookup(storage, &key)) == NULL) {
        z_message_set_state(msg, STORAGE_STATE_NOT_EXISTS);
        return(1);
    }

    if (!(item->flags & MEMCACHE_ITEM_NUMBER)) {
        z_message_set_state(msg, STORAGE_STATE_MEMCACHE_NOT_NUMBER);
        return(2);
    }

    switch (z_message_type(msg)) {
        case STORAGE_MEMCACHE_INCR:
            item->data.number += delta;
            break;
        case STORAGE_MEMCACHE_DECR:
            if (item->data.number > delta)
                item->data.number -= delta;
            else
                item->data.number = 0;
            break;
        default:
            break;
    }

    /* Fill Response */
    z_message_response_stream(msg, &stream);
    z_message_set_state(msg, STORAGE_STATE_STORED);
    z_stream_write_uint64(&stream, item->data.number);

    return(0);
}

static int __storage_delete (struct storage *storage, z_message_t *msg) {
    memcache_object_t *item;
    z_stream_extent_t key;
    z_stream_t stream;
    uint32_t klength;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(&stream, &klength);
    z_stream_set_extent(&stream, &key, stream.offset, klength);

    if ((item = __item_lookup(storage, &key)) == NULL) {
        z_message_set_state(msg, STORAGE_STATE_NOT_EXISTS);
        return(1);
    }

    __item_remove(storage, item);
    z_message_set_state(msg, STORAGE_STATE_DELETED);

    return(0);
}

static int __storage_flush (struct storage *storage,
                            z_message_t *msg)
{
    __items_clear(storage);
    return(0);
}

int storage_request_exec (struct storage *storage, z_message_t *msg) {
    switch (z_message_type(msg)) {
        case STORAGE_MEMCACHE_GET:
        case STORAGE_MEMCACHE_BGET:
        case STORAGE_MEMCACHE_GETS:
            __storage_get(storage, msg);
            break;
        case STORAGE_MEMCACHE_ADD:
        case STORAGE_MEMCACHE_SET:
        case STORAGE_MEMCACHE_REPLACE:
        case STORAGE_MEMCACHE_PREPEND:
        case STORAGE_MEMCACHE_APPEND:
        case STORAGE_MEMCACHE_CAS:
            __storage_set(storage, msg);
            break;
        case STORAGE_MEMCACHE_INCR:
        case STORAGE_MEMCACHE_DECR:
            __storage_arithmetic(storage, msg);
            break;
        case STORAGE_MEMCACHE_DELETE:
            __storage_delete(storage, msg);
            break;
        case STORAGE_MEMCACHE_FLUSH_ALL:
            __storage_flush(storage, msg);
            break;
        default:
            fprintf(stderr, "Unknown command: %u\n", z_message_type(msg));
            break;
    }

    return(0);
}

