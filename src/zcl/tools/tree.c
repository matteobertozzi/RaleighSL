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

#include <zcl/tree.h>

/* ===========================================================================
 *  PRIVATE Tree & Tree Iterator methods
 */
static void *__tree_iter_lookup_node (z_tree_iter_t *iter,
                                      z_tree_node_t *key)
{
    z_compare_t key_compare;
    z_tree_node_t *node;
    void *user_data;
    int cmp;

    iter->height = 0;
    iter->current = NULL;
    node = iter->tree->root;
    user_data = iter->tree->info.user_data;
    key_compare = iter->tree->info.key_compare;

    while (node != NULL) {
        if (node == key) {
            iter->current = node;
            return(node->data);
        }

        iter->stack[iter->height++] = node;
        cmp = key_compare(user_data, node->data, key->data);
        node = node->child[cmp < 0];
    }

    iter->height = 0;
    return(NULL);
}

static void *__tree_iter_lookup_child (z_tree_iter_t *iter,
                                       int child)
{
    z_tree_node_t *node;

    iter->height = 0;
    iter->current = NULL;
    node = iter->tree->root;

    while (node != NULL) {
        if (node->child[child] == NULL) {
            iter->current = node;
            return(node->data);
        }

        iter->stack[iter->height++] = node;
        node = node->child[child];
    }

    return(NULL);
}

void *__tree_iter_lookup_near (z_tree_iter_t *iter,
                               z_compare_t key_compare,
                               const void *key,
                               int ceil)
{
    z_tree_node_t *node;
    int nceil = !ceil;
    void *user_data;
    int cmp;

    iter->height = 0;
    iter->current = NULL;
    node = iter->tree->root;
    user_data = iter->tree->info.user_data;

    while (node != NULL) {
        iter->stack[++(iter->height)] = node;

        if (!(cmp = key_compare(user_data, node->data, key))) {
            iter->current = node;
            return(node->data);
        }

        if (ceil ? (cmp > 0) : (cmp < 0)) {
            if (node->child[nceil] != NULL) {
                node = node->child[nceil];
            } else {
                iter->current = node;
                return(node->data);
            }
        } else {
            if (node->child[ceil] != NULL) {
                node = node->child[ceil];
            } else {
                z_tree_node_t *parent = iter->stack[--(iter->height)];
                while (parent != NULL && node == parent->child[ceil]) {
                    node = parent;
                    parent = iter->stack[--(iter->height)];
                }
                iter->current = parent;
                return((parent != NULL) ? parent->data : NULL);
            }
        }
    }

    return(NULL);
}

static void *__tree_iter_traverse (z_tree_iter_t *iter,
                                   int dir)
{
    z_tree_node_t *node;

    if ((node = iter->current) == NULL)
        return(NULL);

    if (node->child[dir] != NULL) {
        int ndir = !dir;

        iter->stack[iter->height++] = node;
        node = node->child[dir];

        while (node->child[ndir] != NULL) {
            iter->stack[iter->height++] = node;
            node = node->child[ndir];
        }
    } else {
        z_tree_node_t *tmp;

        do {
            if (iter->height == 0) {
                iter->current = NULL;
                return(NULL);
            }

            tmp = node;
            node = iter->stack[--(iter->height)];
        } while (tmp == node->child[dir]);
    }

    iter->current = node;
    return(node->data);
}

static void *__tree_lookup_child (const z_tree_t *tree,
                                  int child)
{
    z_tree_node_t *node;

    node = tree->root;
    while (node != NULL) {
        if (node->child[child] == NULL)
            return(node->data);

        node = node->child[child];
    }

    return(NULL);
}

static void __tree_clear (z_tree_t *tree,
                          z_tree_node_t *node)
{
    if (node != NULL) {
        __tree_clear(tree, node->child[0]);
        __tree_clear(tree, node->child[1]);

        if (tree->info.data_free != NULL)
            tree->info.data_free(tree->info.user_data, node->data);

        z_object_struct_free(tree, z_tree_node_t, node);
    }
}

/* ===========================================================================
 *  PUBLIC Tree methods
 */
z_tree_t *z_tree_alloc (z_tree_t *tree,
                        z_memory_t *memory,
                        z_tree_plug_t *plug,
                        z_compare_t key_compare,
                        z_mem_free_t data_free,
                        void *user_data)
{
    if ((tree = z_object_alloc(memory, tree, z_tree_t)) == NULL)
        return(NULL);

    if (plug == NULL)
        plug = &z_tree_avl;

    if (z_tree_init(tree, plug, key_compare, data_free, user_data)) {
        z_object_free(tree);
        return(NULL);
    }

    return(tree);
}

int z_tree_init (z_tree_t *tree,
                 z_tree_plug_t *plug,
                 z_compare_t key_compare,
                 z_mem_free_t data_free,
                 void *user_data)
{
    tree->info.plug = plug;
    tree->info.data_free = data_free;
    tree->info.key_compare = key_compare;
    tree->info.user_data = user_data;
    tree->root = NULL;
    tree->size = 0U;
    return(0);
}

void z_tree_free (z_tree_t *tree) {
    z_tree_clear(tree);
    z_object_free(tree);
}

int z_tree_clear (z_tree_t *tree) {
    __tree_clear(tree, tree->root);
    tree->root = NULL;
    tree->size = 0U;
    return(0);
}

#define __tree_insert(tree, data)                                           \
    (tree)->info.plug->insert(&((tree)->info), z_object_memory(tree),       \
                              &((tree)->root), data)

#define __tree_remove(tree, key)                                            \
    (tree)->info.plug->remove(&((tree)->info), z_object_memory(tree),       \
                              &((tree)->root), key)

int z_tree_insert (z_tree_t *tree,
                   void *data)
{
    int r;
    if (!(r = __tree_insert(tree, data)))
        tree->size++;
    return(r < 0);
}

int z_tree_remove (z_tree_t *tree,
                   const void *key)
{
    int r;
    if (!(r = __tree_remove(tree, key)))
        tree->size--;
    return(r);
}

int z_tree_remove_min (z_tree_t *tree) {
    void *key;

    if ((key = z_tree_lookup_min(tree)) == NULL)
        return(1);

    return(z_tree_remove(tree, key));
}

int z_tree_remove_max (z_tree_t *tree) {
    void *key;

    if ((key = z_tree_lookup_max(tree)) == NULL)
        return(1);

    return(z_tree_remove(tree, key));
}

int z_tree_remove_range (z_tree_t *tree,
                         const void *min_key,
                         const void *max_key)
{
    z_compare_t key_compare;
    z_tree_node_t *min_node;
    z_tree_iter_t iter;
    void *user_data;
    int cmp;

    z_tree_iter_init(&iter, tree);
    z_tree_iter_lookup(&iter, min_key);
    min_node = iter.current;

    key_compare = tree->info.key_compare;
    user_data = tree->info.user_data;

    while (z_tree_iter_next(&iter)) {
        if ((cmp = key_compare(user_data, iter.current->data, max_key)) >= 0) {
            /* if (!cmp)
                tree->plug->remove(tree, iter.current->data); */
            break;
        }

        tree->size--;
        __tree_remove(tree, iter.current->data);
        __tree_iter_lookup_node(&iter, min_node);
    }

    /* if (min_node != NULL)
        tree->plug->remove(tree, min_node->data); */

    return(0);
}

int z_tree_remove_index (z_tree_t *tree,
                         unsigned long start,
                         unsigned long length)
{
    z_tree_node_t *next;
    z_tree_iter_t iter;
    unsigned long i;
    void *key;

    z_tree_iter_init(&iter, tree);

    /* Loop until i == start */
    key = z_tree_iter_lookup_min(&iter);
    for (i = 0; i < start && key != NULL; ++i)
        key = z_tree_iter_next(&iter);

    /* Loop for length items */
    for (i = 0; i < length && key != NULL; ++i) {
        z_tree_iter_next(&iter);
        next = iter.current;

        tree->size--;
        __tree_remove(tree, key);

        if (next != NULL)
            key = __tree_iter_lookup_node(&iter, next);
    }

    return(0);
}

void *z_tree_lookup (const z_tree_t *tree,
                     const void *key)
{
    return(z_tree_lookup_custom(tree, tree->info.key_compare, key));
}

void *z_tree_lookup_ceil (const z_tree_t *tree,
                          const void *key)
{
    z_tree_iter_t iter;
    z_tree_iter_init(&iter, tree);
    return(z_tree_iter_lookup_ceil(&iter, key));
}

void *z_tree_lookup_floor (const z_tree_t *tree,
                           const void *key)
{
    z_tree_iter_t iter;
    z_tree_iter_init(&iter, tree);
    return(z_tree_iter_lookup_floor(&iter, key));
}

void *z_tree_lookup_custom (const z_tree_t *tree,
                            z_compare_t key_compare,
                            const void *key)
{
    z_tree_node_t *node;
    void *user_data;
    int cmp;

    node = tree->root;
    user_data = tree->info.user_data;
    while (node != NULL) {
        if (!(cmp = key_compare(user_data, node->data, key)))
            return(node->data);

        node = node->child[cmp < 0];
    }

    return(NULL);
}

void *z_tree_lookup_ceil_custom (const z_tree_t *tree,
                                 z_compare_t key_compare,
                                 const void *key)
{
    z_tree_iter_t iter;
    z_tree_iter_init(&iter, tree);
    return(z_tree_iter_lookup_floor_custom(&iter, key_compare, key));
}

void *z_tree_lookup_floor_custom (const z_tree_t *tree,
                                  z_compare_t key_compare,
                                  const void *key)
{
    z_tree_iter_t iter;
    z_tree_iter_init(&iter, tree);
    return(z_tree_iter_lookup_floor_custom(&iter, key_compare, key));
}

void *z_tree_lookup_min (const z_tree_t *tree) {
    return(__tree_lookup_child(tree, 0));
}

void *z_tree_lookup_max (const z_tree_t *tree) {
    return(__tree_lookup_child(tree, 1));
}

/* ===========================================================================
 *  PUBLIC Tree Iterator methods
 */
int z_tree_iter_init (z_tree_iter_t *iter,
                      const z_tree_t *tree)
{
    iter->tree = tree;
    iter->current = NULL;
    iter->height = 0;
    return(0);
}

void *z_tree_iter_lookup (z_tree_iter_t *iter,
                          const void *key)
{
    return(z_tree_iter_lookup_custom(iter, iter->tree->info.key_compare, key));
}

void *z_tree_iter_lookup_ceil (z_tree_iter_t *iter,
                               const void *key)
{
    return(z_tree_iter_lookup_ceil_custom(iter, iter->tree->info.key_compare, key));
}

void *z_tree_iter_lookup_floor (z_tree_iter_t *iter,
                                const void *key)
{
    return(z_tree_iter_lookup_floor_custom(iter, iter->tree->info.key_compare, key));
}

void *z_tree_iter_lookup_custom (z_tree_iter_t *iter,
                                 z_compare_t key_compare,
                                 const void *key)
{
    z_tree_node_t *node;
    void *user_data;
    int cmp;

    iter->height = 0;
    iter->current = NULL;
    node = iter->tree->root;
    user_data = iter->tree->info.user_data;

    while (node != NULL) {
        if (!(cmp = key_compare(user_data, node->data, key))) {
            iter->current = node;
            return(node->data);
        }

        iter->stack[iter->height++] = node;
        node = node->child[cmp < 0];
    }

    iter->height = 0;
    return(NULL);
}

void *z_tree_iter_lookup_ceil_custom (z_tree_iter_t *iter,
                                      z_compare_t key_compare,
                                      const void *key)
{
    return(__tree_iter_lookup_near(iter, key_compare, key, 1));
}

void *z_tree_iter_lookup_floor_custom (z_tree_iter_t *iter,
                                       z_compare_t key_compare,
                                       const void *key)
{
    return(__tree_iter_lookup_near(iter, key_compare, key, 0));
}

void *z_tree_iter_lookup_min (z_tree_iter_t *iter) {
    return(__tree_iter_lookup_child(iter, 0));
}

void *z_tree_iter_lookup_max (z_tree_iter_t *iter) {
    return(__tree_iter_lookup_child(iter, 1));
}

void *z_tree_iter_next (z_tree_iter_t *iter) {
    return(__tree_iter_traverse(iter, 1));
}

void *z_tree_iter_prev (z_tree_iter_t *iter) {
    return(__tree_iter_traverse(iter, 0));
}
