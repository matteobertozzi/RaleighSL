/*
 *   Copyright 2011-2013 Matteo Bertozzi
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
 *  PRIVATE Skip-List data structures
 */
struct z_skip_list_node {
    void *data;
    z_skip_list_node_t *next[0];
};

/* ===========================================================================
 *  PRIVATE Skip-List macros
 */
#define Z_SKIP_LIST_MAX_HEIGHT      8

#define __skip_node_size(level)                                             \
    (sizeof(z_skip_list_node_t) + ((level) * sizeof(z_skip_list_node_t *)))

#define __skip_head_alloc(skip)                                             \
    __skip_node_alloc(skip, Z_SKIP_LIST_MAX_HEIGHT, NULL)

#define __skip_list_seed(skip)                                              \
    z_object_flags(skip).uflags32

#define __skip_list_set_seed(skip, seed)                                    \
    z_object_flags(skip).uflags32 = (seed)

/* ===========================================================================
 *  PRIVATE Skip-List utils
 */
static unsigned int __skip_rand (z_skip_list_t *skip) {
    unsigned int next = __skip_list_seed(skip);
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

    __skip_list_set_seed(skip, next);
    return(result);
}

static unsigned int __skip_rand_level (z_skip_list_t *skip) {
    unsigned int level = 1;

    while (level < Z_SKIP_LIST_MAX_HEIGHT && (__skip_rand(skip) & 0x3) == 0)
        level++;

    return(level);
}

/* ===========================================================================
 *  PRIVATE Skip-List methods
 */
static z_skip_list_node_t *__skip_node_alloc (z_skip_list_t *skip,
                                              unsigned int level,
                                              void *data)
{
    z_skip_list_node_t *node;

    node = z_object_block_alloc(skip, z_skip_list_node_t, __skip_node_size(level));
    if (Z_UNLIKELY(node == NULL))
        return(NULL);

    node->data = data;
    while (level--)
        node->next[level] = NULL;

    return(node);
}

static void __skip_node_free (z_skip_list_t *skip, z_skip_list_node_t *node) {
    if (skip->data_free != NULL)
        skip->data_free(skip->user_data, node->data);

    z_object_block_free(skip, node);
}

static z_skip_list_node_t *__skip_node_min (const z_skip_list_t *self) {
    return(self->head->next[0]);
}

static z_skip_list_node_t *__skip_node_max (const z_skip_list_t *self) {
    z_skip_list_node_t *next;
    z_skip_list_node_t *p;
    unsigned int level;

    p = self->head;
    level = self->levels;
    while (level--) {
        while ((next = p->next[level]) != NULL)
            p = next;
    }

    return((p == self->head) ? NULL : p);
}

static z_skip_list_node_t *__skip_node_lookup (const z_skip_list_t *skip,
                                               z_compare_t key_compare,
                                               const void *key)
{
    z_skip_list_node_t *next;
    unsigned int level;
    z_skip_list_node_t *p;
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

static z_skip_list_node_t *__skip_node_less (const z_skip_list_t *skip,
                                             z_compare_t key_compare,
                                             const void *key)
{
    z_skip_list_node_t *next;
    z_skip_list_node_t *p;
    unsigned int level;

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

static z_skip_list_node_t *__skip_node_find (const z_skip_list_t *skip,
                                             z_compare_t key_compare,
                                             const void *key,
                                             z_skip_list_node_t **prev,
                                             int *equals)
{
    z_skip_list_node_t *next;
    unsigned int level;
    z_skip_list_node_t *p;
    int cmp = 1;

    p = skip->head;
    level = skip->levels;
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
void *z_skip_list_get (z_skip_list_t *self, const void *key) {
    return(z_skip_list_get_custom(self, self->key_compare, key));
}

void *z_skip_list_get_custom (z_skip_list_t *self, z_compare_t key_compare, const void *key) {
    z_skip_list_node_t *node;

    if ((node = __skip_node_lookup(self, key_compare, key)) != NULL)
        return(node->data);

    return(NULL);
}

int z_skip_list_put (z_skip_list_t *self, void *key_value) {
    z_skip_list_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT];
    unsigned int i, level;
    z_skip_list_node_t *p;
    int equals;

    p = __skip_node_find(self, self->key_compare, key_value, prev, &equals);
    if (p != NULL && equals) {
        if (self->data_free != NULL && key_value != p->data)
            self->data_free(self->user_data, p->data);

        p->data = key_value;
        return(0);
    }

    if ((level = __skip_rand_level(self)) > self->levels) {
        for (i = self->levels; i < level; ++i)
            prev[i] = self->head;
        self->levels = level;
    }

    if ((p = __skip_node_alloc(self, level, key_value)) == NULL)
        return(1);

    for (i = 0; i < level; ++i) {
        p->next[i] = prev[i]->next[i];
        prev[i]->next[i] = p;
    }

    return(0);
}

void *z_skip_list_pop (z_skip_list_t *self, const void *key) {
    return(z_skip_list_pop_custom(self, self->key_compare, key));
}

void *z_skip_list_pop_custom (z_skip_list_t *self, z_compare_t key_compare, const void *key) {
    z_skip_list_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT];
    z_skip_list_node_t *p;
    unsigned int i;
    void *item;
    int equals;

    p = __skip_node_find(self, key_compare, key, prev, &equals);
    if (p == NULL || !equals)
        return(NULL);

    for (i = 0; i < self->levels; ++i) {
        if (prev[i]->next[i] == p)
            prev[i]->next[i] = p->next[i];
    }

    while (self->levels > 1 && self->head->next[self->levels - 1] == NULL)
        self->levels--;

    item = p->data;
    __skip_node_free(self, p);
    return(item);
}

int z_skip_list_remove (z_skip_list_t *self, const void *key) {
    return(z_skip_list_remove_custom(self, self->key_compare, key));
}

int z_skip_list_remove_custom (z_skip_list_t *self, z_compare_t key_compare, const void *key) {
    void *item;

    if ((item = z_skip_list_pop_custom(self, key_compare, key)) != NULL) {
        if (self->data_free != NULL)
            self->data_free(self->user_data, item);
        return(1);
    }

    return(0);
}

void z_skip_list_clear (z_skip_list_t *self) {
    z_skip_list_node_t *next;
    z_skip_list_node_t *p;

    for (p = self->head->next[0]; p != NULL; p = next) {
        next = p->next[0];
        if (self->data_free != NULL)
            self->data_free(self->user_data, p->data);
        __skip_node_free(self, p);
    }

    self->levels = 1;
}

int z_skip_list_size (const z_skip_list_t *self) {
    return(self->size);
}

void *z_skip_list_min (const z_skip_list_t *self) {
    const z_skip_list_node_t *node;

    if ((node = __skip_node_min(self)) != NULL)
        return(node->data);

    return(NULL);
}

void *z_skip_list_max (const z_skip_list_t *self) {
    const z_skip_list_node_t *node;

    if ((node = __skip_node_max(self)) != NULL)
        return(node->data);

    return(NULL);
}

void *z_skip_list_ceil (const z_skip_list_t *self, const void *key) {
    z_skip_list_node_t *node;
    node = __skip_node_find(self, self->key_compare, key, NULL, NULL);
    return((node != NULL) ? node->data : NULL);
}

void *z_skip_list_floor (const z_skip_list_t *self, const void *key) {
    z_skip_list_node_t *node;
    if ((node = __skip_node_less(self, self->key_compare, key)) != NULL)
        return(node->data);
    return(NULL);
}

/* ===========================================================================
 *  INTERFACE Map methods
 */
static void *__skip_list_get (void *self, const void *key) {
    return(z_skip_list_get(Z_SKIP_LIST(self), key));
}

static void *__skip_list_get_custom (void *self, z_compare_t key_compare, const void *key) {
    return(z_skip_list_get_custom(Z_SKIP_LIST(self), key_compare, key));
}

static int __skip_list_put (void *self, void *key_value) {
    return(z_skip_list_put(Z_SKIP_LIST(self), key_value));
}

static void *__skip_list_pop (void *self, const void *key) {
    return(z_skip_list_pop(Z_SKIP_LIST(self), key));
}

static void *__skip_list_pop_custom (void *self, z_compare_t key_compare, const void *key) {
    return(z_skip_list_pop_custom(Z_SKIP_LIST(self), key_compare, key));
}

static int __skip_list_remove (void *self, const void *key) {
    return(z_skip_list_remove(Z_SKIP_LIST(self), key));
}

static int __skip_list_remove_custom (void *self, z_compare_t key_compare, const void *key) {
    return(z_skip_list_remove_custom(Z_SKIP_LIST(self), key_compare, key));
}

static void __skip_list_clear (void *self) {
    z_skip_list_clear(Z_SKIP_LIST(self));
}

static int __skip_list_size (const void *self) {
    return(z_skip_list_size(Z_CONST_SKIP_LIST(self)));
}

/* ===========================================================================
 *  INTERFACE SortedMap methods
 */
static void *__skip_list_min (const void *self) {
    return(z_skip_list_min(Z_CONST_SKIP_LIST(self)));
}

static void *__skip_list_max (const void *self) {
    return(z_skip_list_max(Z_CONST_SKIP_LIST(self)));
}

static void *__skip_list_ceil (const void *self, const void *key) {
    return(z_skip_list_ceil(Z_CONST_SKIP_LIST(self), key));
}

static void *__skip_list_floor (const void *self, const void *key) {
    return(z_skip_list_floor(Z_CONST_SKIP_LIST(self), key));
}

/* ===========================================================================
 *  INTERFACE Iterator methods
 */
static int __skip_list_open (void *self, const void *object) {
    z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
    Z_ITERATOR_INIT(iter, Z_CONST_SKIP_LIST(object));
    iter->current = __skip_node_min(Z_CONST_SKIP_LIST(object));
    return(0);
}

static void *__skip_list_begin (void *self) {
    z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
    const z_skip_list_t *skip = Z_CONST_SKIP_LIST(z_iterator_object(iter));
    if ((iter->current = __skip_node_min(skip)) != NULL)
        return(iter->current->data);
    return(NULL);
}

static void *__skip_list_end (void *self) {
    z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
    const z_skip_list_t *skip = Z_CONST_SKIP_LIST(z_iterator_object(iter));
    if ((iter->current = __skip_node_max(skip)) != NULL)
        return(iter->current->data);
    return(NULL);
}

static void *__skip_list_next (void *self) {
    z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
    if (Z_LIKELY(iter->current != NULL)) {
        if ((iter->current = iter->current->next[0]) != NULL)
            return(iter->current->data);
    }
    return(NULL);
}

static void *__skip_list_prev (void *self) {
    z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
    const z_skip_list_t *skip = Z_CONST_SKIP_LIST(z_iterator_object(iter));

    if (iter->current == NULL)
        return(NULL);

    iter->current = __skip_node_less(skip, skip->key_compare, iter->current->data);
    return((iter->current != NULL) ? iter->current->data : NULL);
}

static void *__skip_list_skip (void *self, long n) {
    void *ret = NULL;
    /* TODO: avoid calling iter functions */
    if (n > 0) {
        while (n-- > 0)
            ret = __skip_list_next(self);
    } else {
        while (n-- > 0)
            ret = __skip_list_prev(self);
    }
    return ret;
}

static void *__skip_list_current (void *self) {
    z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
    return((iter->current != NULL) ? iter->current->data : NULL);
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __skip_list_ctor (void *self, z_memory_t *memory, va_list args) {
    z_skip_list_t *skip = Z_SKIP_LIST(self);

    if ((skip->head = __skip_head_alloc(skip)) == NULL)
        return(1);

    skip->key_compare = va_arg(args, z_compare_t);
    skip->data_free = va_arg(args, z_mem_free_t);
    skip->user_data = va_arg(args, void *);
    __skip_list_set_seed(skip, va_arg(args, unsigned int));
    skip->levels = 1;

    return(0);
}

static void __skip_list_dtor (void *self) {
    __skip_list_clear(self);
}

/* ===========================================================================
 *  SkipList vtables
 */
static const z_vtable_type_t __skip_list_type = {
    .name = "SkipList",
    .size = sizeof(z_skip_list_t),
    .ctor = __skip_list_ctor,
    .dtor = __skip_list_dtor,
};

static const z_vtable_map_t __skip_list_map = {
    .get            = __skip_list_get,
    .put            = __skip_list_put,
    .pop            = __skip_list_pop,
    .remove         = __skip_list_remove,
    .clear          = __skip_list_clear,
    .size           = __skip_list_size,

    .get_custom     = __skip_list_get_custom,
    .pop_custom     = __skip_list_pop_custom,
    .remove_custom  = __skip_list_remove_custom,
};

static const z_vtable_sorted_map_t __skip_list_sorted_map = {
    .min    = __skip_list_min,
    .max    = __skip_list_max,
    .ceil   = __skip_list_ceil,
    .floor  = __skip_list_floor,
};

static const z_vtable_iterator_t __skip_list_iterator = {
    .open    = __skip_list_open,
    .close   = NULL,
    .begin   = __skip_list_begin,
    .end     = __skip_list_end,
    .skip    = __skip_list_skip,
    .next    = __skip_list_next,
    .prev    = __skip_list_prev,
    .current = __skip_list_current,
};

static const z_sorted_map_interfaces_t __skip_list_interfaces = {
    .type       = &__skip_list_type,
    .iterator   = &__skip_list_iterator,
    .map        = &__skip_list_map,
    .sorted_map = &__skip_list_sorted_map,
};

/* ===========================================================================
 *  PUBLIC Skip-List constructor/destructor
 */
z_skip_list_t *z_skip_list_alloc (z_skip_list_t *self,
                                  z_memory_t *memory,
                                  z_compare_t key_compare,
                                  z_mem_free_t data_free,
                                  void *user_data,
                                  unsigned int seed)
{
    return(z_object_alloc(self, &__skip_list_interfaces, memory,
                          key_compare, data_free, user_data, seed));
}

void z_skip_list_free (z_skip_list_t *self) {
    z_object_free(self);
}

