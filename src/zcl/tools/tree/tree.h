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

#include <zcl/macros.h>
#include <zcl/map.h>

#ifndef Z_TREE_MAX_HEIGHT
    #define Z_TREE_MAX_HEIGHT               (32)
#endif /* !Z_TREE_MAX_HEIGHT */

Z_TYPEDEF_STRUCT(z_tree_plug)
Z_TYPEDEF_STRUCT(z_tree_node)
Z_TYPEDEF_STRUCT(z_tree_info)
Z_TYPEDEF_STRUCT(z_tree_iter)
Z_TYPEDEF_STRUCT(z_tree)

#define Z_TREE_TYPE                     z_tree_t __base__;
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

  int             (*insert)      (const z_tree_info_t *tree,
                                  z_tree_node_t **root,
                                  void *data);
  int             (*remove)      (const z_tree_info_t *tree,
                                  z_tree_node_t **root,
                                  const void *key);
};

#define z_tree_node_attach(tree, root, node)                                  \
  (tree)->plug->attach(tree, root, node)

#define z_tree_node_detach(tree, root, key)                                   \
  (tree)->plug->detach(tree, root, key)

#define z_tree_node_detach_min(tree, root)                                    \
  (tree)->plug->detach_edge(tree, root, 0)

#define z_tree_node_detach_max(tree, root)                                    \
  (tree)->plug->detach_edge(tree, root, 1)

#define z_tree_node_insert(tree, root, data)                                  \
  (tree)->plug->insert(tree, root, data)

#define z_tree_node_remove(tree, root, data)                                  \
  (tree)->plug->remove(tree, root, data)

const z_tree_plug_t z_tree_red_black;
const z_tree_plug_t z_tree_avl;

__Z_END_DECLS__

#endif /* _Z_TREE_H_ */
