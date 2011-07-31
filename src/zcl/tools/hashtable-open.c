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

struct node {
    void *data;
    unsigned int hash;
};

#define __HASH_TABLE_BUCKET(table)          Z_CAST(struct node, table->bucket)

#define __entry_match(table, key_compare, entry, key_hash, key)             \
    ((entry)->hash == key_hash && (entry)->data != NULL &&                  \
     !key_compare((table)->user_data, (entry)->data, key))

#define __node_lookup(table, key)                                           \
    __node_lookup_custom(table, (table)->hash_func,                         \
                         (table)->key_compare, key)

static unsigned int __ilog2 (unsigned int n) {
    unsigned int log2 = 0U;
    if (n & (n - 1)) log2 += 1;
    if (n >> 16) { log2 += 16; n >>= 16; }
    if (n >>  8) { log2 +=  8; n >>= 8; }
    if (n >>  4) { log2 +=  4; n >>= 4; }
    if (n >>  2) { log2 +=  2; n >>= 2; }
    if (n >>  1) { log2 +=  1; }
    return(log2);
}

static struct node *__node_lookup_custom (z_hash_table_t *table,
                                          z_object_hash_t hash_func,
                                          z_compare_t key_compare,
                                          const void *key)
{
    struct node *bucket;
    struct node *entry;
    unsigned int count;
    unsigned int shift;
    unsigned int mask;
    unsigned int hash;
    unsigned int i, p;

    bucket = __HASH_TABLE_BUCKET(table);

    hash = hash_func(table->user_data, key);

    count = table->size;
    shift = __ilog2(count);
    mask = count - 1;
    i = p = hash & mask;
    while (count--) {
        i = (i << 2) + i + p + 1;

        entry = &(bucket[i & mask]);
        if (__entry_match(table, key_compare, entry, hash, key))
            return(entry);

        p >>= shift;
    }

    return(NULL);
}

static int __table_insert (z_hash_table_t *table,
                           unsigned int hash,
                           void *data)
{
    struct node *bucket;
    struct node *ientry;
    struct node *entry;
    unsigned int count;
    unsigned int shift;
    unsigned int mask;
    unsigned int i, p;

    bucket = __HASH_TABLE_BUCKET(table);
    ientry = NULL;

    count = table->size;
    shift = __ilog2(count);
    mask = count - 1;
    i = p = hash & mask;
    while (count--) {
        i = (i << 2) + i + p + 1;

        entry = &(bucket[i & mask]);
        if (ientry == NULL && entry->data == NULL) {
            ientry = entry;
        } else if (__entry_match(table, table->key_compare, entry, hash,data)) {
            if (table->data_free != NULL && data != entry->data)
                table->data_free(table->user_data, entry->data);
            entry->data = data;
            return(0);
        }

        p >>= shift;
    }

    if (ientry != NULL) {
        ientry->data = data;
        ientry->hash = hash;
        return(0);
    }

    return(1);
}

static int __open_table_init (z_hash_table_t *table, unsigned int size) {
    if (!(table->bucket = z_object_array_zalloc(table, size, struct node)))
        return(1);

    return(0);
}

static void __open_table_uninit (z_hash_table_t *table) {
    z_object_array_free(table, table->bucket);
}

static int __open_table_insert (z_hash_table_t *table, void *data) {
    unsigned int hash;

    hash = table->hash_func(table, data);
    return(__table_insert(table, hash, data));
}

static int __open_table_remove (z_hash_table_t *table, const void *key) {
    struct node *entry;

    if ((entry = __node_lookup(table, key)) == NULL)
        return(1);

    if (table->data_free != NULL)
        table->data_free(table->user_data, entry->data);

    entry->data = NULL;
    entry->hash = 0U;

    return(0);
}

static void __open_table_clear (z_hash_table_t *table) {
    struct node *bucket;
    struct node *entry;
    unsigned int count;

    bucket = __HASH_TABLE_BUCKET(table);
    count = table->size;
    while (count--) {
        entry = &(bucket[count]);
        if (entry->data == NULL)
            continue;

        if (table->data_free != NULL)
            table->data_free(table->user_data, entry->data);

        entry->data = NULL;
        entry->hash = 0U;
    }

    table->used = 0U;
}

static int __open_table_resize (z_hash_table_t *table, unsigned int new_size) {
    struct node *new_bucket;
    struct node *bucket;
    struct node *entry;
    unsigned int size;

    if (!(new_bucket = z_object_array_zalloc(table, new_size, struct node)))
        return(1);

    size = table->size;
    bucket = __HASH_TABLE_BUCKET(table);

    table->size = new_size;
    table->bucket = new_bucket;

    while (size--) {
        entry = &(bucket[size]);

        if (entry->data != NULL)
            __table_insert(table, entry->hash, entry->data);
    }

    z_object_array_free(table, bucket);

    return(0);
}

static void *__open_table_lookup (z_hash_table_t *table,
                                  z_object_hash_t hash_func,
                                  z_compare_t key_compare,
                                  const void *key)
{
    struct node *entry;

    if ((entry = __node_lookup_custom(table, hash_func, key_compare, key)))
        return(entry->data);

    return(NULL);
}

static void __open_table_foreach (z_hash_table_t *table,
                                  z_foreach_t func,
                                  void *user_data)
{
    struct node *bucket;
    struct node *entry;
    unsigned int count;

    bucket = __HASH_TABLE_BUCKET(table);
    count = table->size;
    while (count--) {
        entry = &(bucket[count]);
        if (entry->data == NULL)
            continue;

        if (!func(user_data, entry->data))
            return;
    }
}

z_hash_table_plug_t z_hash_table_open = {
    .init       = __open_table_init,
    .uninit     = __open_table_uninit,
    .insert     = __open_table_insert,
    .remove     = __open_table_remove,
    .clear      = __open_table_clear,
    .resize     = __open_table_resize,
    .lookup     = __open_table_lookup,
    .foreach    = __open_table_foreach,
};

