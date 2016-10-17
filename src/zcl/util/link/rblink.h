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

#ifndef _Z_RB_LINK_H_
#define _Z_RB_LINK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_rblink z_rblink_t;
typedef struct z_rbtree z_rbtree_t;
typedef struct z_rb_iter z_rb_iter_t;

typedef int (*z_rb_node_cmp_t) (void *udata, const z_rblink_t *a, const z_rblink_t *b);
typedef int (*z_rb_key_cmp_t)  (void *udata, const void *k, const z_rblink_t *n);

struct z_rblink {
  z_rblink_t *child[2];
	z_rblink_t *parent;
  uint8_t  color;
  uint8_t  udata8;
  uint16_t udata16;
  uint32_t udata32;
};

struct z_rbtree {
  z_rblink_t *root;
};

struct z_rb_iter {
  z_rblink_t *current;
  z_rbtree_t *tree;
  int found;
};

void          z_rb_tree_init    (z_rbtree_t *self);
z_rblink_t *  z_rb_tree_insert  (z_rbtree_t *self,
                                 z_rb_node_cmp_t cmp_func,
                                 z_rblink_t *node,
                                 void *udata);
int           z_rb_tree_remove  (z_rbtree_t *self,
                                 z_rb_key_cmp_t cmp_func,
                                 const void *key,
                                 void *udata);
z_rblink_t *  z_rb_tree_lookup  (const z_rbtree_t *self,
                                 z_rb_key_cmp_t key_cmp,
                                 const void *key,
                                 void *udata);

z_rblink_t *z_rb_link_next       (z_rblink_t *node);
z_rblink_t *z_rb_link_prev       (z_rblink_t *node);
int         z_rb_link_has_next   (z_rblink_t *node);
int         z_rb_link_has_prev   (z_rblink_t *node);

void        z_rb_iter_init       (z_rb_iter_t *self, z_rbtree_t *tree);
z_rblink_t *z_rb_iter_seek_begin (z_rb_iter_t *self);
z_rblink_t *z_rb_iter_seek_end   (z_rb_iter_t *self);
z_rblink_t *z_rb_iter_seek_le    (z_rb_iter_t *iter,
                                  z_rb_key_cmp_t key_cmp,
                                  const void *key,
                                  void *udata);
z_rblink_t *z_rb_iter_seek_ge    (z_rb_iter_t *iter,
                                  z_rb_key_cmp_t key_cmp,
                                  const void *key,
                                  void *udata);
z_rblink_t *z_rb_iter_next       (z_rb_iter_t *self);
z_rblink_t *z_rb_iter_prev       (z_rb_iter_t *self);

__Z_END_DECLS__

#endif /* !_Z_RB_LINK_H_ */
