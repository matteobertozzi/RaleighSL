/*
 *   Copyright 2011 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <zcl/debug.h>
#include <zcl/tree.h>

/* ===========================================================================
 *  PRIVATE Tree Iterator methods
 */
static const z_tree_node_t *__tree_iter_lookup_near (z_tree_iter_t *iter,
                                                     z_compare_t key_compare,
                                                     const void *key,
                                                     void *udata,
                                                     int ceil_entry)
{
  int nceil_entry = !ceil_entry;
  const z_tree_node_t *node;

  iter->height = 0;
  iter->current = NULL;
  node = iter->root;

  while (node != NULL) {
    int cmp = key_compare(udata, node, key);
    if (cmp == 0) {
      iter->current = node;
      return(node);
    }

    if (ceil_entry ? (cmp > 0) : (cmp < 0)) {
      if (node->child[nceil_entry] != NULL) {
        iter->stack[iter->height++] = node;
        node = node->child[nceil_entry];
      } else {
        iter->current = node;
        return(node);
      }
    } else {
      if (node->child[ceil_entry] != NULL) {
        iter->stack[iter->height++] = node;
        node = node->child[ceil_entry];
      } else {
        /* TODO: FIX ME */
        const z_tree_node_t *parent = (iter->height > 0) ? iter->stack[--(iter->height)] : NULL;
        while (parent != NULL && node == parent->child[ceil_entry]) {
          node = parent;
          parent = (iter->height > 0) ? iter->stack[--(iter->height)] : NULL;
        }
        iter->current = parent;
        return(parent);
      }
    }
  }

  return(NULL);
}

static const z_tree_node_t *__tree_iter_lookup_child (z_tree_iter_t *iter, int child) {
  const z_tree_node_t *node;

  node = iter->root;
  iter->current = NULL;
  iter->height = 0;

  if (node != NULL) {
    while (node->child[child] != NULL) {
      iter->stack[iter->height++] = node;
      node = node->child[child];
    }
  }

  iter->current = node;
  return(node);
}

static const z_tree_node_t *__tree_iter_traverse (z_tree_iter_t *iter, int dir) {
  const z_tree_node_t *node;

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
    const z_tree_node_t *tmp;

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
  return(node);
}

/* ===========================================================================
 *  PUBLIC Tree Node methods
 */
int z_tree_node_levels (const z_tree_node_t *root) {
  if (root != NULL) {
    int levels[2];
    levels[0] = z_tree_node_levels(root->child[0]);
    levels[1] = z_tree_node_levels(root->child[1]);
    return(1 + z_max(levels[0], levels[1]));
  }
  return(0);
}

z_tree_node_t *z_tree_node_lookup (z_tree_node_t *root,
                                   z_compare_t key_compare,
                                   const void *key,
                                   void *udata)
{
  while (root != NULL) {
    int cmp;

    if (!(cmp = key_compare(udata, root, key)))
      return(root);

    root = root->child[cmp < 0];
  }
  return(NULL);
}

z_tree_node_t *z_tree_node_floor (z_tree_node_t *root,
                                  z_compare_t key_compare,
                                  const void *key,
                                  void *udata)
{
  const z_tree_node_t *node;
  z_tree_iter_t iter;
  z_tree_iter_init(&iter, root);
  node = z_tree_iter_seek_le(&iter, key_compare, key, udata);
  return((z_tree_node_t *)node);
}

z_tree_node_t *z_tree_node_ceil (z_tree_node_t *root,
                                 z_compare_t key_compare,
                                 const void *key,
                                 void *udata)
{
  const z_tree_node_t *node;
  z_tree_iter_t iter;
  z_tree_iter_init(&iter, root);
  node = z_tree_iter_seek_ge(&iter, key_compare, key, udata);
  return((z_tree_node_t *)node);
}

z_tree_node_t *z_tree_node_closest (z_tree_node_t *root,
                                    z_closest_t closest,
                                    const void *key,
                                    void *udata)
{
  uint64_t min_dist = 0xffffffffffffffffull;
  z_tree_node_t *closest_node = NULL;
  z_tree_node_t *node = root;

  while (node != NULL) {
    uint64_t dist = 0;
    int cmp;

    cmp = closest(udata, node, key, &dist);
    if (dist < min_dist) {
      min_dist = dist;
      closest_node = node;
    }

    if (cmp > 0) {
      node = node->child[0];
    } else if (cmp < 0) {
      node = node->child[1];
    } else /* if (cmp == 0) */ {
      break;
    }
  }
  return(closest_node);
}

void z_tree_node_clear (z_tree_node_t *node, z_mem_free_t free_func, void *udata) {
  if (node != NULL) {
    z_tree_node_clear(node->child[0], free_func, udata);
    z_tree_node_clear(node->child[1], free_func, udata);
    free_func(udata, node);
  }
}

/* ===========================================================================
 *  PUBLIC Tree Node methods
 */
void z_tree_iter_init (z_tree_iter_t *iter, const z_tree_node_t *root) {
  iter->root = root;
  iter->current = NULL;
  iter->height = 0;
}

const z_tree_node_t *z_tree_iter_seek_le (z_tree_iter_t *iter,
                                          z_compare_t key_compare,
                                          const void *key,
                                          void *udata)
{
  const z_tree_node_t *node;
  if (key == NULL)
    return(__tree_iter_lookup_child(iter, 0));
  node = __tree_iter_lookup_near(iter, key_compare, key, udata, 0);
  return(node != NULL ? node : __tree_iter_lookup_child(iter, 0));
}

const z_tree_node_t *z_tree_iter_seek_ge (z_tree_iter_t *iter,
                                          z_compare_t key_compare,
                                          const void *key,
                                          void *udata)
{
  if (key == NULL)
    return(__tree_iter_lookup_child(iter, 0));
  return(__tree_iter_lookup_near(iter, key_compare, key, udata, 1));
}

const z_tree_node_t *z_tree_iter_begin (z_tree_iter_t *iter) {
  return(__tree_iter_lookup_child(iter, 0));
}

const z_tree_node_t *z_tree_iter_end (z_tree_iter_t *iter) {
  return(__tree_iter_lookup_child(iter, 1));
}

const z_tree_node_t *z_tree_iter_next (z_tree_iter_t *iter) {
  return(__tree_iter_traverse(iter, 1));
}

const z_tree_node_t *z_tree_iter_prev (z_tree_iter_t *iter) {
  return(__tree_iter_traverse(iter, 0));
}
