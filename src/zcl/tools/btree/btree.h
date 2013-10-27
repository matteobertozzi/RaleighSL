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

#ifndef _Z_BTREE_H_
#define _Z_BTREE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/bytes.h>

Z_TYPEDEF_STRUCT(z_vtable_btree_node)
Z_TYPEDEF_STRUCT(z_btree_item)
Z_TYPEDEF_STRUCT(z_btree)

typedef int (*z_btree_item_func_t) (void *udata, z_btree_item_t *item);

struct z_vtable_btree_node {
  int   (*open)         (uint8_t *node, uint32_t size);
  void  (*create)       (uint8_t *node, uint32_t size);
  void  (*finalize)     (uint8_t *node);

  int   (*has_space)    (const uint8_t *node, const z_btree_item_t *item);
  int   (*append)       (uint8_t *node, const z_btree_item_t *item);

  int   (*remove)       (uint8_t *node,
                         const void *key, size_t ksize,
                         z_btree_item_t *item);

  int   (*fetch_first)  (const uint8_t *node,
                         z_btree_item_t *item);
  int   (*fetch_next)   (const uint8_t *node,
                         z_btree_item_t *item);
};

struct z_btree_item {
  const uint8_t *key;
  const uint8_t *value;
  uint32_t kprefix;
  uint32_t ksize;
  uint32_t vsize;
  uint32_t index;
  uint8_t  is_deleted;
};

struct z_btree {
  void *root;
  unsigned int nnodes;
  unsigned int height;
};

extern const z_vtable_btree_node_t z_btree_vnode;
extern const z_vtable_btree_node_t z_btree_xnode;

int z_btree_node_search (const uint8_t *node,
                         const z_vtable_btree_node_t *vtable,
                         const void *key, size_t ksize,
                         z_btree_item_t *item);

void z_btree_node_merge (const uint8_t *node_a,
                         const z_vtable_btree_node_t *vtable_a,
                         const uint8_t *node_b,
                         const z_vtable_btree_node_t *vtable_b,
                         z_btree_item_func_t item_func,
                         void *udata);
void z_btree_node_multi_merge (const uint8_t *nodes[],
                               unsigned int nnodes,
                               const z_vtable_btree_node_t *vtable,
                               z_btree_item_func_t item_func,
                               void *udata);

int z_btree_open (z_btree_t *btree);
void z_btree_close (z_btree_t *btree);
int z_btree_insert (z_btree_t *btree, z_bytes_t *key, z_bytes_t *value);
z_bytes_t *z_btree_remove (z_btree_t *self, z_bytes_t *key);
z_bytes_t *z_btree_lookup (const z_btree_t *self, const z_bytes_t *key);
void z_btree_debug (const z_btree_t *btree);

__Z_END_DECLS__

#endif /* !_Z_BTREE_H_ */
