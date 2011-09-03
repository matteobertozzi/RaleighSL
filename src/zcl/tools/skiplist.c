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

#include <zcl/skiplist.h>

/* ===========================================================================
 *  PRIVATE Skip-List macros
 */
#define __skip_node_size(level)                                             \
    (sizeof(z_skip_node_t) + (((level) - 1) * sizeof(z_skip_node_t *)))

#define __skip_head_alloc(skip)                                             \
    __skip_node_alloc(skip, Z_SKIP_LIST_MAX_HEIGHT, NULL)

/* ===========================================================================
 *  PRIVATE Skip-List methods
 */
static z_skip_node_t *__skip_node_alloc (z_skip_list_t *skip,
                                         unsigned int level,
                                         void *data)
{
    z_skip_node_t *node;

    node = z_object_block_alloc(skip, z_skip_node_t, __skip_node_size(level));
    if (node == NULL)
        return(NULL);

    node->data = data;
    while (level--)
        node->next[level] = NULL;

    return(node);
}

static void __skip_node_free (z_skip_list_t *skip, z_skip_node_t *node) {
    if (skip->data_free != NULL)
        skip->data_free(skip->user_data, node->data);

    z_object_block_free(skip, node);
}

static unsigned int __skip_rand (z_skip_list_t *skip) {
    unsigned int next = skip->seed;
    unsigned int result;

    next *= 1103515245;
    next += 12345;
    result = (next >> 16) & 2047;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (next >> 16) & 1023;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (next >> 16) & 1023;

    skip->seed = next;

    return(result);
}

static unsigned int __skip_rand_level (z_skip_list_t *skip) {
    unsigned int level = 1;

    while (level < Z_SKIP_LIST_MAX_HEIGHT && (__skip_rand(skip) & 0x3) == 0)
        level++;

    return(level);
}

static z_skip_node_t *__skip_min (const z_skip_list_t *skip) {
    return(skip->head->next[0]);
}

static z_skip_node_t *__skip_max (const z_skip_list_t *skip) {
    z_skip_node_t *next;
    unsigned int level;
    z_skip_node_t *p;

    p = skip->head;
    level = skip->levels;
    while (level--) {
        while ((next = p->next[level]) != NULL)
            p = next;
    }

    return((p == skip->head) ? NULL : p);
}


static z_skip_node_t *__skip_lookup (const z_skip_list_t *skip,
                                     z_compare_t key_compare,
                                     const void *key)
{
    z_skip_node_t *next;
    unsigned int level;
    z_skip_node_t *p;
    int cmp;

    p = skip->head;
    level = skip->levels;
    while (level--) {
        while ((next = p->next[level]) != NULL) {
            if (!(cmp = key_compare(skip->user_data, next->data, key)))
                return(next);

            if (cmp > 0)
                break;

            p = next;
        }
    }

    return(NULL);
}

static z_skip_node_t *__skip_node_less (const z_skip_list_t *skip,
                                        z_compare_t key_compare,
                                        const void *key)
{
    z_skip_node_t *next;
    unsigned int level;
    z_skip_node_t *p;

    p = skip->head;
    level = skip->levels;
    while (level--) {
        while ((next = p->next[level]) != NULL) {
            if (key_compare(skip->user_data, next->data, key) >= 0) {
                if (level == 0)
                    return(p);
                break;
            }

            p = next;
        }
    }

    return(p);
}

static z_skip_node_t *__skip_node_find (const z_skip_list_t *skip,
                                        const void *key,
                                        z_skip_node_t **prev,
                                        int *equals)
{
    z_compare_t key_compare;
    z_skip_node_t *next;
    unsigned int level;
    z_skip_node_t *p;
    int cmp = 1;

    p = skip->head;
    level = skip->levels;
    key_compare = skip->key_compare;
    while (level--) {
        while ((next = p->next[level]) != NULL) {
            if ((cmp = key_compare(skip->user_data, next->data, key)) >= 0) {
                if (prev != NULL)
                    prev[level] = p;

                if (level == 0) {
                    if (equals != NULL)
                        *equals = (cmp == 0);
                    return(next);
                }

                break;
            }

            p = next;
        }

        if (prev != NULL)
            prev[level] = p;
    }

    if (equals != NULL)
        *equals = 0;
    return(p);
}

/* ===========================================================================
 *  PUBLIC Skip-List methods
 */
z_skip_list_t *z_skip_list_alloc (z_skip_list_t *skip,
                                  z_memory_t *memory,
                                  z_compare_t key_compare,
                                  z_mem_free_t data_free,
                                  void *user_data)
{
    if ((skip = z_object_alloc(memory, skip, z_skip_list_t)) == NULL)
        return(NULL);

    if ((skip->head = __skip_head_alloc(skip)) == NULL) {
        z_object_free(skip);
        return(NULL);
    }

    skip->key_compare = key_compare;
    skip->data_free = data_free;
    skip->user_data = user_data;

    skip->levels = 1;
    skip->seed = 0x7bcb4948;

    return(skip);
}

void z_skip_list_free (z_skip_list_t *skip) {
    z_skip_list_clear(skip);
    z_object_free(skip);
}

int z_skip_list_insert (z_skip_list_t *skip, void *data) {
    z_skip_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT];
    unsigned int i, level;
    z_skip_node_t *p;
    int equals;

    p = __skip_node_find(skip, data, prev, &equals);
    if (p != NULL && equals) {
        if (skip->data_free != NULL && data != p->data)
            skip->data_free(skip->user_data, p->data);

        p->data = data;
        return(0);
    }

    if ((level = __skip_rand_level(skip)) > skip->levels) {
        for (i = skip->levels; i < level; ++i)
            prev[i] = skip->head;
        skip->levels = level;
    }

    if ((p = __skip_node_alloc(skip, level, data)) == NULL)
        return(1);

    for (i = 0; i < level; ++i) {
        p->next[i] = prev[i]->next[i];
        prev[i]->next[i] = p;
    }

    return(0);
}

int z_skip_list_remove (z_skip_list_t *skip, const void *key) {
    z_skip_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT];
    z_skip_node_t *p;
    unsigned int i;
    int equals;

    p = __skip_node_find(skip, key, prev, &equals);
    if (p == NULL || !equals)
        return(1);

    for (i = 0; i < skip->levels; ++i) {
        if (prev[i]->next[i] == p)
            prev[i]->next[i] = p->next[i];
    }

    while (skip->levels > 1 && skip->head->next[skip->levels - 1] == NULL)
        skip->levels--;

    __skip_node_free(skip, p);
    return(0);
}

void z_skip_list_clear (z_skip_list_t *skip) {
    z_skip_node_t *next;
    z_skip_node_t *p;

    for (p = skip->head->next[0]; p != NULL; p = next) {
        next = p->next[0];
        __skip_node_free(skip, p);
    }

    skip->levels = 1;
}

void *z_skip_list_lookup (const z_skip_list_t *skip, const void *key) {
    return(z_skip_list_lookup_custom(skip, skip->key_compare, key));
}

void *z_skip_list_lookup_custom (const z_skip_list_t *skip,
                                 z_compare_t key_compare,
                                 const void *key)
{
    z_skip_node_t *node;

    if ((node = __skip_lookup(skip, key_compare, key)) != NULL)
        return(node->data);

    return(NULL);
}

void *z_skip_list_lookup_min (const z_skip_list_t *skip) {
    if (skip->head->next[0] != NULL)
        return(skip->head->next[0]->data);
    return(NULL);
}

void *z_skip_list_lookup_max (const z_skip_list_t *skip) {
    const z_skip_node_t *next;
    const z_skip_node_t *p;
    unsigned int level;

    p = skip->head;
    level = skip->levels - 1;
    for (;;) {
        next = p->next[level];
        if (next != NULL) {
            p = next;
        } else {
            if (level == 0)
                return(p->data);
            level--;
        }
    }

    return(NULL);
}


/* ===========================================================================
 *  PUBLIC Skip-List Iterator methods
 */
int z_skip_iter_init (z_skip_iter_t *iter, const z_skip_list_t *skip) {
    iter->skip = skip;
    iter->current = NULL;
    return(0);
}

void *z_skip_iter_next (z_skip_iter_t *iter) {
    if (iter->current != NULL) {
        iter->current = iter->current->next[0];
        return((iter->current != NULL) ? iter->current->data : NULL);
    }
    return(NULL);
}

void *z_skip_iter_prev (z_skip_iter_t *iter) {
    if (iter->current == NULL)
        return(NULL);

    iter->current = __skip_node_less(iter->skip,
                                     iter->skip->key_compare,
                                     iter->current->data);
    return((iter->current != NULL) ? iter->current->data : NULL);
}

void *z_skip_iter_lookup (z_skip_iter_t *iter, const void *key) {
    return(z_skip_iter_lookup_custom(iter, iter->skip->key_compare, key));
}

void *z_skip_iter_lookup_custom (z_skip_iter_t *iter,
                                 z_compare_t key_compare,
                                 const void *key)
{
    if ((iter->current = __skip_lookup(iter->skip, key_compare, key)) != NULL)
        return(iter->current->data);
    return(NULL);
}

void *z_skip_iter_lookup_min (z_skip_iter_t *iter) {
    if ((iter->current = __skip_min(iter->skip)) != NULL)
        return(iter->current->data);
    return(NULL);
}

void *z_skip_iter_lookup_max (z_skip_iter_t *iter) {
    if ((iter->current = __skip_max(iter->skip)) != NULL)
        return(iter->current->data);
    return(NULL);
}

