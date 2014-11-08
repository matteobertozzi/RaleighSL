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

#ifndef _Z_AVL16S_H_
#define _Z_AVL16S_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_avl16_head)
Z_TYPEDEF_STRUCT(z_avl16_node)
Z_TYPEDEF_STRUCT(z_avl16_iter)

struct z_avl16_head {
  uint16_t root;
  uint16_t edge[2];
} __attribute__((packed));

struct z_avl16_node {
  uint16_t child[2];
  int8_t   balance : 3;
  uint8_t  length  : 5;
  uint8_t  data[1];
} __attribute__((packed));

struct z_avl16_iter {
  uint8_t *block;
  uint16_t stack[18];
  uint16_t current;
  uint16_t height;
  uint16_t root;
};

typedef int (*z_avl16_compare_t)  (void *udata,
                                   const z_avl16_node_t *node,
                                   const void *key);

#define Z_AVL16_POS(block, node)  (Z_CAST(uint8_t, node) - (block))
#define Z_AVL16_NODE(block, pos)  Z_CAST(z_avl16_node_t, (block) + (pos))
#define Z_AVL16_NODE_SIZE         (sizeof(z_avl16_node_t) - 1)

void *   z_avl16_insert           (z_avl16_head_t *head,
                                   uint8_t *block,
                                   uint16_t node_pos,
                                   z_avl16_compare_t cmpfunc,
                                   const void *key,
                                   void *udata);
void     z_avl16_add_edge         (z_avl16_head_t *head,
                                   uint8_t *block,
                                   uint16_t node_pos,
                                   const int edge);
#define z_avl16_prepend(h, b, n)  z_avl16_add_edge(h, b, n, 0)
#define z_avl16_append(h, b, n)   z_avl16_add_edge(h, b, n, 1)

uint16_t z_avl16_remove           (z_avl16_head_t *head,
                                   uint8_t *block,
                                   z_avl16_compare_t key_cmp,
                                   const void *key,
                                   void *udata);
uint16_t z_avl16_remove_edge      (z_avl16_head_t *head,
                                   uint8_t *block,
                                   const int edge);
#define  z_avl16_remove_min(h, b) z_avl16_remove_edge(h, b, 0)
#define  z_avl16_remove_max(h, b) z_avl16_remove_edge(h, b, 1)

void *   z_avl16_lookup           (uint8_t *block,
                                   uint16_t root,
                                   z_avl16_compare_t key_cmp,
                                   const void *key,
                                   void *udata);
uint16_t z_avl16_lookup_edge      (uint8_t *block,
                                   uint16_t root,
                                   int edge);
#define  z_avl16_lookup_min(b, r) z_avl16_lookup_edge(b, r, 0)
#define  z_avl16_lookup_max(b, r) z_avl16_lookup_edge(b, r, 1)

void     z_avl16_iter_init        (z_avl16_iter_t *self,
                                   uint8_t *block,
                                   uint16_t root);
void *   z_avl16_iter_seek_begin  (z_avl16_iter_t *self);
void *   z_avl16_iter_seek_end    (z_avl16_iter_t *self);
void *   z_avl16_iter_seek_le     (z_avl16_iter_t *iter,
                                   z_avl16_compare_t key_cmp,
                                   const void *key,
                                   void *udata);
void *   z_avl16_iter_seek_ge     (z_avl16_iter_t *iter,
                                   z_avl16_compare_t key_cmp,
                                   const void *key,
                                   void *udata);
void *   z_avl16_iter_next        (z_avl16_iter_t *self);
void *   z_avl16_iter_prev        (z_avl16_iter_t *self);

__Z_END_DECLS__

#endif /* !_Z_AVL16S_H_ */
