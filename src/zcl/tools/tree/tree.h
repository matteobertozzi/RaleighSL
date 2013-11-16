/*
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

#ifndef _Z_TREE_H_
#define _Z_TREE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memory.h>
#include <zcl/macros.h>
#include <zcl/map.h>

#ifndef Z_TREE_MAX_HEIGHT
    #define Z_TREE_MAX_HEIGHT               (32 + 1)
#endif /* !Z_TREE_MAX_HEIGHT */

Z_TYPEDEF_STRUCT(z_tree_plug)
Z_TYPEDEF_STRUCT(z_tree_node)
Z_TYPEDEF_STRUCT(z_tree_info)
Z_TYPEDEF_STRUCT(z_tree_iter)
Z_TYPEDEF_STRUCT(z_tree)

#define __Z_TREE_NODE__                 z_tree_node_t __node__;
#define Z_TREE_NODE(x)                  Z_CAST(z_tree_node_t, x)
#define Z_TREE(x)                       Z_CAST(z_tree_t, x)

struct z_tree_node {
  z_tree_node_t *child[2];
  void *         data;
  int            balance;
};

struct z_tree_info {
  const z_tree_plug_t *plug;
  z_compare_t    key_compare;
  z_mem_free_t   data_free;
  void *         user_data;
};

struct z_tree {
  __Z_OBJECT__(z_type)

  z_tree_node_t *root;
  z_tree_info_t  info;

  unsigned long  size;
};

struct z_tree_iter {
  z_tree_node_t *     stack[Z_TREE_MAX_HEIGHT];
  z_tree_node_t *     current;
  const z_tree_t *    tree;
  unsigned int        height;
};

struct z_tree_plug {
  int             (*attach)      (const z_tree_info_t *tree,
                                  z_tree_node_t **root,
                                  z_tree_node_t *node);
  z_tree_node_t * (*detach)      (const z_tree_info_t *tree,
                                  z_tree_node_t **root,
                                  const void *key);
  z_tree_node_t * (*detach_edge) (const z_tree_info_t *tree,
                                  z_tree_node_t **root,
                                  int edge);
};

#define z_tree_node_attach(tree, root, node)                                  \
  (tree)->plug->attach(tree, root, node)

#define z_tree_node_detach(tree, root, key)                                   \
  (tree)->plug->detach(tree, root, key)

#define z_tree_node_detach_min(tree, root)                                    \
  (tree)->plug->detach_edge(tree, root, 0)

#define z_tree_node_detach_max(tree, root)                                    \
  (tree)->plug->detach_edge(tree, root, 1)

const z_tree_plug_t z_tree_red_black;
const z_tree_plug_t z_tree_avl;

int                  z_tree_node_levels (const z_tree_node_t *root);
const z_tree_node_t *z_tree_node_lookup (const z_tree_node_t *root,
                                         z_compare_t key_compare,
                                         const void *key,
                                         void *udata);

/* Tree methods */
z_tree_t *  z_tree_alloc                (z_tree_t *tree,
                                         const z_tree_plug_t *plug,
                                         z_compare_t key_compare,
                                         z_mem_free_t free_data,
                                         void *user_data);
void        z_tree_free                 (z_tree_t *tree);

int         z_tree_insert               (z_tree_t *tree,
                                         void *data);
int         z_tree_remove               (z_tree_t *tree,
                                         const void *key);
int         z_tree_remove_min           (z_tree_t *tree);
int         z_tree_remove_max           (z_tree_t *tree);
int         z_tree_remove_range         (z_tree_t *tree,
                                         const void *min_key,
                                         const void *max_key);
int         z_tree_remove_index         (z_tree_t *tree,
                                         unsigned long start,
                                         unsigned long length);
int         z_tree_clear                (z_tree_t *tree);

void *      z_tree_lookup               (const z_tree_t *tree,
                                         const void *key);
void *      z_tree_lookup_ceil          (const z_tree_t *tree,
                                         const void *key);
void *      z_tree_lookup_floor         (const z_tree_t *tree,
                                         const void *key);
void *      z_tree_lookup_custom        (const z_tree_t *tree,
                                         z_compare_t key_compare,
                                         const void *key);
void *      z_tree_lookup_ceil_custom   (const z_tree_t *tree,
                                         z_compare_t key_compare,
                                         const void *key);
void *      z_tree_lookup_floor_custom  (const z_tree_t *tree,
                                         z_compare_t key_compare,
                                         const void *key);
void *      z_tree_lookup_min           (const z_tree_t *tree);
void *      z_tree_lookup_max           (const z_tree_t *tree);

/* Tree Iterator methods */
int         z_tree_iter_init                (z_tree_iter_t *iter,
                                             const z_tree_t *tree);

void *      z_tree_iter_next                 (z_tree_iter_t *iter);
void *      z_tree_iter_prev                 (z_tree_iter_t *iter);

void *      z_tree_iter_lookup               (z_tree_iter_t *iter,
                                              const void *key);
void *      z_tree_iter_lookup_ceil          (z_tree_iter_t *iter,
                                              const void *key);
void *      z_tree_iter_lookup_floor         (z_tree_iter_t *iter,
                                              const void *key);
void *      z_tree_iter_lookup               (z_tree_iter_t *iter,
                                              const void *key);
void *      z_tree_iter_lookup_custom        (z_tree_iter_t *iter,
                                              z_compare_t key_compare,
                                              const void *key);
void *      z_tree_iter_lookup_ceil_custom   (z_tree_iter_t *iter,
                                              z_compare_t key_compare,
                                              const void *key);
void *      z_tree_iter_lookup_floor_custom  (z_tree_iter_t *iter,
                                              z_compare_t key_compare,
                                              const void *key);
void *      z_tree_iter_lookup_min            (z_tree_iter_t *iter);
void *      z_tree_iter_lookup_max            (z_tree_iter_t *iter);

__Z_END_DECLS__

#endif /* _Z_TREE_H_ */
