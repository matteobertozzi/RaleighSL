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

#ifndef _Z_AVL_LINK_H_
#define _Z_AVL_LINK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_avl_link z_avl_link_t;
typedef struct z_avl_tree z_avl_tree_t;
typedef struct z_avl_iter z_avl_iter_t;

typedef int (*z_avl_node_cmp_t) (void *udata, const z_avl_link_t *a, const z_avl_link_t *b);
typedef int (*z_avl_key_cmp_t)  (void *udata, const void *k, const z_avl_link_t *n);

struct z_avl_link {
  z_avl_link_t *child[2];
  int8_t   balance;
  uint8_t  udata8;
  uint16_t udata16;
  uint32_t udata32;
};

struct z_avl_tree {
  z_avl_link_t *root;
};

struct z_avl_iter {
  z_avl_link_t *stack[32];
  z_avl_link_t *current;
  z_avl_link_t *root;
  int height;
  int found;
};

void          z_avl_tree_init       (z_avl_tree_t *self);
z_avl_link_t *z_avl_tree_insert     (z_avl_tree_t *self,
                                     z_avl_node_cmp_t cmp_func,
                                     z_avl_link_t *node,
                                     void *udata);
int           z_avl_tree_remove     (z_avl_tree_t *self,
                                     z_avl_key_cmp_t cmp_func,
                                     const void *key,
                                     void *udata);
z_avl_link_t *z_avl_tree_lookup     (const z_avl_tree_t *self,
                                     z_avl_key_cmp_t key_cmp,
                                     const void *key,
                                     void *udata);

void          z_avl_iter_init       (z_avl_iter_t *self, z_avl_tree_t *tree);
z_avl_link_t *z_avl_iter_seek_begin (z_avl_iter_t *self);
z_avl_link_t *z_avl_iter_seek_end   (z_avl_iter_t *self);
z_avl_link_t *z_avl_iter_seek_le    (z_avl_iter_t *iter,
                                     z_avl_key_cmp_t key_cmp,
                                     const void *key,
                                     void *udata);
z_avl_link_t *z_avl_iter_seek_ge    (z_avl_iter_t *iter,
                                     z_avl_key_cmp_t key_cmp,
                                     const void *key,
                                     void *udata);
z_avl_link_t *z_avl_iter_next       (z_avl_iter_t *self);
z_avl_link_t *z_avl_iter_prev       (z_avl_iter_t *self);

__Z_END_DECLS__

#endif /* !_Z_AVL_LINK_H_ */
