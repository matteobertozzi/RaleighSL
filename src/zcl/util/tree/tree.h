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

#include <zcl/comparer.h>
#include <zcl/macros.h>
#include <zcl/memory.h>

#ifndef Z_TREE_MAX_HEIGHT
    #define Z_TREE_MAX_HEIGHT               (32 + 1)
#endif /* !Z_TREE_MAX_HEIGHT */

Z_TYPEDEF_STRUCT(z_tree_plug)
Z_TYPEDEF_STRUCT(z_tree_node)
Z_TYPEDEF_STRUCT(z_tree_info)
Z_TYPEDEF_STRUCT(z_tree_iter)

struct z_tree_node {
  z_tree_node_t *child[2];
  int8_t   balance;
  uint8_t  uflags8;
  uint16_t uflags16;
  uint32_t uflags32;
};

struct z_tree_info {
  const z_tree_plug_t *plug;
  z_compare_t  node_compare;
  z_compare_t  key_compare;
  z_mem_free_t node_free;
};

struct z_tree_iter {
  const z_tree_node_t *stack[Z_TREE_MAX_HEIGHT];
  const z_tree_node_t *current;
  const z_tree_node_t *root;
  const z_tree_info_t *tree;
  unsigned int height;
};

struct z_tree_plug {
  int             (*attach)      (const z_tree_info_t *tree,
                                  z_tree_node_t **root,
                                  z_tree_node_t *node,
                                  void *udata);
  z_tree_node_t * (*detach)      (const z_tree_info_t *tree,
                                  z_tree_node_t **root,
                                  const void *key,
                                  void *udata);
  z_tree_node_t * (*detach_edge) (const z_tree_info_t *tree,
                                  z_tree_node_t **root,
                                  int edge);
};

extern const z_tree_plug_t z_tree_avl;

#define z_tree_node_attach(tree, root, node, udata)                           \
  (tree)->plug->attach(tree, root, node, udata)

#define z_tree_node_detach(tree, root, key, udata)                            \
  (tree)->plug->detach(tree, root, key, udata)

#define z_tree_node_detach_min(tree, root)                                    \
  (tree)->plug->detach_edge(tree, root, 0)

#define z_tree_node_detach_max(tree, root)                                    \
  (tree)->plug->detach_edge(tree, root, 1)


int               z_tree_node_levels    (const z_tree_node_t *root);
z_tree_node_t *   z_tree_node_lookup    (z_tree_node_t *root,
                                         z_compare_t key_compare,
                                         const void *key,
                                         void *udata);
z_tree_node_t *   z_tree_node_floor     (z_tree_node_t *root,
                                         z_compare_t key_compare,
                                         const void *key,
                                         void *udata);
z_tree_node_t *   z_tree_node_ceil      (z_tree_node_t *root,
                                         z_compare_t key_compare,
                                         const void *key,
                                         void *udata);
z_tree_node_t *   z_tree_node_closest   (z_tree_node_t *root,
                                         z_closest_t closest,
                                         const void *key,
                                         void *udata);
void              z_tree_node_clear     (z_tree_node_t *node,
                                         z_mem_free_t free_func,
                                         void *udata);

void  z_tree_iter_init  (z_tree_iter_t *iter,
                         const z_tree_node_t *root);

const z_tree_node_t *z_tree_iter_seek_le (z_tree_iter_t *iter,
                                          z_compare_t key_compare,
                                          const void *key,
                                          void *udata);
const z_tree_node_t *z_tree_iter_seek_ge (z_tree_iter_t *iter,
                                          z_compare_t key_compare,
                                          const void *key,
                                          void *udata);
const z_tree_node_t *z_tree_iter_begin   (z_tree_iter_t *iter);
const z_tree_node_t *z_tree_iter_end     (z_tree_iter_t *iter);
const z_tree_node_t *z_tree_iter_next    (z_tree_iter_t *iter);
const z_tree_node_t *z_tree_iter_prev    (z_tree_iter_t *iter);

__Z_END_DECLS__

#endif /* _Z_TREE_H_ */
