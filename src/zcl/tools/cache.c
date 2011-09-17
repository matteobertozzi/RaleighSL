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

#include <zcl/cache.h>

#define __cache_data_free(cache, node_data)                                   \
    do {                                                                      \
        if ((node_data) != NULL && (cache)->free_func != NULL)                \
            (cache)->free_func((cache)->user_data, node_data);                \
    } while (0)

#define __cache_data_equals(cache, func, data_a, data_b)                      \
    (!(func)((cache)->user_data, data_a, data_b))

#define __cache_data_hash(cache, func, key)                                   \
    ((func)((cache)->user_data, key) & ((cache)->size - 1))

static z_cache_node_t *__cache_node_add (z_cache_t *cache) {
    z_cache_node_t *head;
    z_cache_node_t *node;

    if ((node = z_object_struct_alloc(cache, z_cache_node_t)) == NULL)
        return(NULL);

    node->data = NULL;
    if ((head = cache->head) != NULL) {
        node->prev = head->prev;
        node->next = head;
        head->prev = node;
        node->prev->next = node;
    } else {
        node->prev = node;
        node->next = node;
    }

    cache->head = node;

    return(node);
}

static void __cache_node_free (z_cache_t *cache, z_cache_node_t *node) {
    __cache_data_free(cache, node->data);
    z_object_struct_free(cache, z_cache_node_t, node);
}

z_cache_node_t *__cache_node_lookup (z_cache_t *cache,
                                     z_compare_t cmp_func,
                                     unsigned int hash_index,
                                     const void *key)
{
    z_cache_node_t *p;

    for (p = cache->table[hash_index]; p != NULL; p = p->hash) {
        if (__cache_data_equals(cache, cmp_func, p->data, key))
            return(p);
    }

    return(NULL);
}

static void __cache_node_unlink (z_cache_t *cache,
                                 unsigned int hash_index,
                                 z_cache_node_t *node)
{
    z_cache_node_t *p;

    if ((p = cache->table[hash_index]) == node) {
        cache->table[hash_index] = NULL;
    } else {
        for (; p != NULL; p = p->hash) {
            if (p->hash == node) {
                p->hash = node->hash;
                break;
            }
        }
    }
}

static void __cache_node_eject (z_cache_t *cache,
                                unsigned int hash_index,
                                z_cache_node_t *node)
{
    __cache_node_unlink(cache, hash_index, node);

    if (cache->head->prev != node) {
        z_cache_node_t *tail;

        node->prev->next = node->next;
        node->next->prev = node->prev;

        if (cache->head == node)
            cache->head = node->next;

        tail = cache->head->prev;
        tail->next = node;
        node->next = cache->head;
        node->prev = tail;
        cache->head->prev = node;
    }

    /* Mark block as free */
    node->data = NULL;
}

z_cache_t *z_cache_alloc (z_cache_t *cache,
                          z_memory_t *memory,
                          unsigned int size,
                          z_object_hash_t hash_func,
                          z_compare_t cmp_func,
                          z_mem_free_t free_func,
                          void *user_data)
{
    if ((cache = z_object_alloc(memory, cache, z_cache_t)) == NULL)
        return(NULL);

    size = z_align_up(size, 8);
    if (!(cache->table = z_object_array_zalloc(cache, size, z_cache_node_t*))) {
        z_object_free(cache);
        return(NULL);
    }

    cache->head = NULL;
    cache->used = 0;
    cache->size = size;

    cache->free_func = free_func;
    cache->hash_func = hash_func;
    cache->cmp_func = cmp_func;
    cache->user_data = user_data;

    if (__cache_node_add(cache) == NULL) {
        z_object_array_free(cache, cache->table);
        z_object_free(cache);
        return(NULL);
    }

    return(cache);
}

void z_cache_free (z_cache_t *cache) {
    if (cache->head != NULL) {
        z_cache_node_t *next;
        z_cache_node_t *p;

        p = cache->head;
        do {
            next = p->next;
            __cache_node_free(cache, p);
        } while ((p = next) != cache->head);
    }

    z_object_array_free(cache, cache->table);
    z_object_free(cache);
}

int z_cache_add (z_cache_t *cache, void *data) {
    z_cache_node_t *node;
    unsigned int index;

    node = cache->head->prev;
    if (cache->head->data == NULL) {
        cache->head = node;
    } else if (cache->used >= cache->size) {
        cache->head = node;

        index = __cache_data_hash(cache, cache->hash_func, node->data);
        __cache_node_unlink(cache, index, node);
        __cache_data_free(cache, node->data);
        cache->used--;
    } else {
        if ((node = __cache_node_add(cache)) == NULL)
            return(1);
    }

    /* Setup Node */
    node->data = data;

    /* Insert node into hashtable */
    index = __cache_data_hash(cache, cache->hash_func, data);
    node->hash = cache->table[index];
    cache->table[index] = node;
    cache->used++;

    return(0);
}

int z_cache_remove (z_cache_t *cache, const void *key) {
    unsigned int hash_index;
    z_cache_node_t *node;

    hash_index = __cache_data_hash(cache, cache->hash_func, key);
    if (!(node = __cache_node_lookup(cache, cache->cmp_func, hash_index, key)))
        return(1);

    __cache_node_eject(cache, hash_index, node);
    cache->used--;

    return(0);
}

void *z_cache_lookup (z_cache_t *cache, const void *key) {
    return(z_cache_lookup_custom(cache,
                                 cache->hash_func,
                                 cache->cmp_func,
                                 key));
}

void *z_cache_lookup_custom (z_cache_t *cache,
                             z_object_hash_t hash_func,
                             z_compare_t cmp_func,
                             const void *key)
{
    unsigned int hash_index;
    z_cache_node_t *node;

    hash_index = __cache_data_hash(cache, hash_func, key);
    node = __cache_node_lookup(cache, cmp_func, hash_index, key);

    return((node != NULL) ? node->data : NULL);
}

