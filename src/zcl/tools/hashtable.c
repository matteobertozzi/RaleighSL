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

#include <zcl/hashtable.h>

unsigned int z_hash_table_grow_policy (void *user_data,
                                       unsigned int used,
                                       unsigned int size)
{
    if (used > (size - (size >> 2)))
        return(size << 1);
    return(size);
}

unsigned int z_hash_table_shrink_policy (void *user_data,
                                         unsigned int used,
                                         unsigned int size)
{
    if (used < (size >> 2) && used > 128)
        return(size >> 1);
    return(size);
}

z_hash_table_t *z_hash_table_alloc (z_hash_table_t *table,
                                    z_memory_t *memory,
                                    z_hash_table_plug_t *plug,
                                    unsigned int size,
                                    z_object_hash_t hash_func,
                                    z_compare_t key_compare,
                                    z_resize_policy_t grow_policy,
                                    z_resize_policy_t shrink_policy,
                                    z_mem_free_t data_free,
                                    void *user_data)
{
    if ((table = z_object_alloc(memory, table, z_hash_table_t)) == NULL)
        return(NULL);

    table->plug = plug;
    table->hash_func = hash_func;
    table->key_compare = key_compare;
    table->grow_policy = grow_policy;
    table->shrink_policy = shrink_policy;
    table->data_free = data_free;
    table->user_data = user_data;

    table->used = 0;
    table->size = z_align_up(size, 16);

    if (plug->init(table, table->size)) {
        z_object_free(table);
        return(NULL);
    }

    return(table);
}

void z_hash_table_free (z_hash_table_t *table) {
    z_hash_table_clear(table);
    table->plug->uninit(table);
    z_object_free(table);
}

int z_hash_table_insert (z_hash_table_t *table, void *data) {
    if (table->grow_policy != NULL) {
        unsigned int n;

        n = table->grow_policy(table->user_data, table->used, table->size);
        if (n != table->size)
            table->plug->resize(table, n);
    }

    return(table->plug->insert(table, data));
}

int z_hash_table_remove (z_hash_table_t *table, const void *key) {
    if (table->plug->remove(table, key))
        return(1);

    if (table->shrink_policy != NULL) {
        unsigned int n;

        n = table->shrink_policy(table->user_data, table->used, table->size);
        if (n != table->size)
           table->plug->resize(table, n);
    }

    return(0);
}

void z_hash_table_clear (z_hash_table_t *table) {
    table->plug->clear(table);
}

void *z_hash_table_lookup (z_hash_table_t *table, const void *key) {
    return(table->plug->lookup(table, table->hash_func,
                               table->key_compare, key));
}

void *z_hash_table_lookup_custom (z_hash_table_t *table,
                                  z_object_hash_t hash_func,
                                  z_compare_t key_compare,
                                  const void *key)
{
    return(table->plug->lookup(table, hash_func, key_compare, key));
}

void z_hash_table_foreach (z_hash_table_t *table,
                           z_foreach_t func,
                           void *user_data)
{
    table->plug->foreach(table, func, user_data);
}

