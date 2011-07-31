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
    struct node *next;
    void *data;
};

#define __HASH_TABLE_BUCKET(table)          Z_CAST(struct node *, table->bucket)

static struct node *__node_alloc (z_hash_table_t *table, void *data) {
    struct node *node;

    if ((node = z_object_struct_alloc(table, struct node)) != NULL) {
        node->data = data;
        node->next = NULL;
    }   

    return(node);
}
 
static void __node_free (z_hash_table_t *table, struct node *node) {
    if (table->data_free != NULL)
        table->data_free(table->user_data, node->data);

    z_object_struct_free(table, struct node, node);
}

static struct node *__node_lookup_custom (z_hash_table_t *table,
                                          z_object_hash_t hash_func,
                                          z_compare_t key_compare,
                                          const void *key)
{
    struct node **bucket;
    struct node *node;
    unsigned int hash;
    int cmp;

    bucket = __HASH_TABLE_BUCKET(table);

    hash = hash_func(table->user_data, key);
    node = bucket[hash & (table->size - 1)];

    while (node != NULL) {
        if (!(cmp = key_compare(table->user_data, node->data, key)))
            return(node);

        if (cmp > 0)
            return(NULL);

        node = node->next;
    }

    return(NULL);
}

static int __node_insert (z_hash_table_t *table, void *data) {
    struct node **bucket;
    unsigned int index;
    struct node *pprev;
    struct node *node;
    struct node *p;
    int cmp;

    bucket = __HASH_TABLE_BUCKET(table);
    index = table->hash_func(table->user_data, data) & (table->size - 1);
    if ((p = bucket[index]) == NULL) {
        if ((node = __node_alloc(table, data)) == NULL)
            return(0);

        bucket[index] = node;
        return(1);
    }

    pprev = p;
    do {
        if (!(cmp = table->key_compare(table->user_data, p->data, data))) {
            if (table->data_free != NULL && p->data != data)
                table->data_free(table->user_data, p->data);

            p->data = data;
            return(2);
        }

        if (cmp > 0)
            break;

        pprev = p;        
    } while ((p = p->next) != NULL);


    /* Insert node */
    if ((node = __node_alloc(table, data)) == NULL)
        return(0);

    if (pprev == p) {
        node->next = bucket[index];
        bucket[index] = node;
    } else {
        node->next = pprev->next;
        pprev->next = node;
    }

    return(3);
}

static int __node_insert_unique (z_hash_table_t *table, struct node *node) {
    struct node **bucket;
    unsigned int index;
    struct node *pprev;
    struct node *p;
    int cmp;

    bucket = __HASH_TABLE_BUCKET(table);
    index = table->hash_func(table->user_data, node->data) & (table->size - 1);
    if ((p = bucket[index]) == NULL) {
        bucket[index] = node;
        node->next = NULL;
        return(1);
    }

    pprev = p;
    do {
        if ((cmp = table->key_compare(table->user_data, p->data, node->data)) > 0)
            break;

        pprev = p;
    } while ((p = p->next) != NULL);

    /* Insert node */
    if (pprev == p) {
        node->next = bucket[index];
        bucket[index] = node;
    } else {
        node->next = pprev->next;
        pprev->next = node;
    }
    return(3);
}


static int __node_remove (z_hash_table_t *table, const void *key) {
    struct node **bucket;
    unsigned int index;
    struct node *pprev;
    struct node *p;
    int cmp;

    bucket = __HASH_TABLE_BUCKET(table);

    index = table->hash_func(table->user_data, key) & (table->size - 1);

    p = pprev = bucket[index];
    while (p != NULL) {
        if (!(cmp = table->key_compare(table->user_data, p->data, key))) {
            if (pprev == bucket[index])
                bucket[index] = p->next;
            else
                pprev->next = p->next;

            __node_free(table, p);
            return(1);
        }

        if (cmp > 0)
            return(0);

        pprev = p;
        p = p->next;
    }

    return(0);
}

static int __chain_table_init (z_hash_table_t *table, unsigned int size) {
    if (!(table->bucket = z_object_array_zalloc(table, size, struct node *)))
        return(1);

    return(0);
}

static void __chain_table_uninit (z_hash_table_t *table) {
    z_object_array_free(table, table->bucket);
}

static int __chain_table_insert (z_hash_table_t *table, void *data) {
    if (__node_insert(table, data)) {
        table->used++;
        return(0);
    }
    return(1);
}

static int __chain_table_remove (z_hash_table_t *table, const void *key) {
    if (__node_remove(table, key)) {
        table->used--;
        return(0);
    }
    return(1);
}

static void __chain_table_clear (z_hash_table_t *table) {
    struct node **bucket;
    unsigned int count;
    struct node *next;
    struct node *p;

    bucket = __HASH_TABLE_BUCKET(table);

    count = table->size;
    while (count--) {
        for (p = bucket[count]; p != NULL; p = next) {
            next = p->next;
            __node_free(table, p);
        }
        bucket[count] = NULL;
    }

    table->used = 0U;
}

static int __chain_table_resize (z_hash_table_t *table, unsigned int new_size) {
    struct node **new_bucket;
    struct node **bucket;
    unsigned int size;
    struct node *next;
    struct node *p;

    if (!(new_bucket = z_object_array_zalloc(table, new_size, struct node *)))
        return(1);

    size = table->size;
    bucket = __HASH_TABLE_BUCKET(table);

    table->size = new_size;
    table->bucket = new_bucket;
    while (size--) {
        for (p = bucket[size]; p != NULL; p = next) {
            next = p->next;
            __node_insert_unique(table, p);
            p->next = NULL;
        }
    }

    z_object_array_free(table, bucket);

    return(0);
}

static void *__chain_table_lookup (z_hash_table_t *table,
                                   z_object_hash_t hash_func,
                                   z_compare_t key_compare,
                                   const void *key)
{
    struct node *node;

    if ((node = __node_lookup_custom(table, hash_func, key_compare, key)))
        return(node->data);

    return(NULL);
}

static void __chain_table_foreach (z_hash_table_t *table,
                                   z_foreach_t func,
                                   void *user_data)
{
    struct node **bucket;
    unsigned int count;
    struct node *next;
    struct node *p;

    bucket = __HASH_TABLE_BUCKET(table);

    count = table->size;
    while (count--) {
        for (p = bucket[count]; p != NULL; p = next) {
            next = p->next;
            if (!func(user_data, p->data))
                return;
        }
    }
}

z_hash_table_plug_t z_hash_table_chain = {
    .init       = __chain_table_init,
    .uninit     = __chain_table_uninit,
    .insert     = __chain_table_insert,
    .remove     = __chain_table_remove,
    .clear      = __chain_table_clear,
    .resize     = __chain_table_resize,
    .lookup     = __chain_table_lookup,
    .foreach    = __chain_table_foreach,
};

