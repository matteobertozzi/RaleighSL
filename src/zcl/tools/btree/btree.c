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

#include <zcl/global.h>
#include <zcl/string.h>
#include <zcl/btree.h>
#include <zcl/debug.h>

#define __NITEMS   8

#define __node_is_full(node)        ((node)->nkeys == __NITEMS)

struct node {
  unsigned int nkeys;
  unsigned int type;
  z_bytes_t *keys[__NITEMS];
  void *values[__NITEMS];
};

static int __key_slot(z_bytes_t **keys,
                      unsigned int nkeys,
                      const z_bytes_t *key)
{
  unsigned int start = 0;
  while (start < nkeys) {
    unsigned int mid = (start & nkeys) + ((start ^ nkeys) >> 1);
    if (z_bytes_compare(keys[mid], key) < 0)
      start = mid + 1;
    else
      nkeys = mid;
  }
  return(start);
}

/* ============================================================================
 *  PRIVATE Tree lookup methods
 */
static struct node *__mtree_find_level (z_btree_t *self,
                                        unsigned int level,
                                        z_bytes_t *key)
{
  struct node *node = (struct node *)self->root;
  unsigned int height;

  for (height = self->height; height > level; height--) {
    struct node *child;
    unsigned int index;

    index = __key_slot(node->keys, node->nkeys, key);
    child = (struct node *)node->values[index];
    /* Key is greater then max update the node */
    if ((index == __NITEMS) || child == NULL) {
      Z_ASSERT(index > 0, "Hit first key");
      index--;
      child = (struct node *)node->values[index];
      if (child != NULL && !__node_is_full(child)) {
        z_bytes_free(node->keys[index]);
        node->keys[index] = z_bytes_acquire(key);
      }
    }

    node = child;
  }
  Z_ASSERT(node != NULL, "Expected at least one node");
  return(node);
}

/* ============================================================================
 *  PRIVATE Node methods
 */
static struct node *__node_alloc (z_btree_t *self, unsigned int level) {
  struct node *node;

  node = z_memory_struct_alloc(z_global_memory(), struct node);
  if (Z_UNLIKELY(node == NULL))
    return(NULL);

  z_memzero(node, sizeof(struct node));
  return(node);
}

static void __node_free (struct node *node) {
  z_memory_struct_free(z_global_memory(), struct node, node);
}

static void __node_insert (struct node *node,
                           unsigned int index,
                           z_bytes_t *key,
                           void *value)
{
  z_memmove(node->keys + index + 1,
            node->keys + index,
            (node->nkeys - index) * sizeof(z_bytes_t *));
  z_memmove(node->values + index + 1,
            node->values + index,
            (node->nkeys - index) * sizeof(void *));

  node->nkeys++;
  node->keys[index] = z_bytes_acquire(key);
  node->values[index] = value;
}

static void __node_remove (struct node *node, unsigned index) {
  node->nkeys--;
  if (node->nkeys == 0)
    return;

  z_memmove(node->keys + index,
            node->keys + index + 1,
            (node->nkeys - index) * sizeof(z_bytes_t *));
  z_memmove(node->values + index,
            node->values + index + 1,
            (node->nkeys - index) * sizeof(void *));
  node->keys[node->nkeys] = NULL;
  node->values[node->nkeys] = NULL;
}

static void __node_split (struct node *node, struct node *new_node) {
  unsigned int hkeys = (__NITEMS >> 1);
  unsigned int ksize = (hkeys * sizeof(void *));
  unsigned int vsize = (hkeys * sizeof(void *));
  void *hkey, *hval;

  hkey = node->keys + hkeys;
  hval = node->values + hkeys;
  z_memcpy(new_node->keys, node->keys, ksize);
  z_memcpy(new_node->values, node->values, vsize);
  z_memcpy(node->keys, hkey, ksize);
  z_memcpy(node->values, hval, vsize);
  z_memset(hkey, 0, ksize);
  z_memset(hval, 0, vsize);

  node->nkeys = hkeys;
  new_node->nkeys = hkeys;
}

/* ============================================================================
 *  PRIVATE Tree insert methods
 */
static int __mtree_grow (z_btree_t *self) {
  struct node *root = (struct node *)self->root;
  struct node *node;

  if ((node = __node_alloc(self, self->height)) == NULL)
    return(-1);

  if (self->root != NULL) {
    z_bytes_t *key = root->keys[root->nkeys - 1];
    node->keys[0] = z_bytes_acquire(key);
    node->values[0] = root;
    node->nkeys = 1;
  }

  self->root = node;
  self->height++;
  return(0);
}

static int __mtree_insert_level (z_btree_t *self,
                                 unsigned int level,
                                 z_bytes_t *key,
                                 void *value)
{
  struct node *node;
  unsigned int index;

  if (self->height < level && __mtree_grow(self))
    return(-1);

_iretry:
  node = __mtree_find_level(self, level, key);
  index = __key_slot(node->keys, node->nkeys, key);
  if (__node_is_full(node)) {
    struct node *new_node;
    z_bytes_t *hkey = key;

    new_node = __node_alloc(self, level + 1);
    if (Z_UNLIKELY(new_node == NULL))
      return(-1);

    /* Split the data */
    if (index > 0 && index < __NITEMS) {
      __node_split(node, new_node);
      hkey = new_node->keys[new_node->nkeys - 1];
    }

    if (__mtree_insert_level(self, level + 1, hkey, new_node)) {
      __node_free(new_node);
      return(-2);
    }

    goto _iretry;
  }

  /* Shift and Insert */
  __node_insert(node, index, key, value);
  return(0);
}

/* ============================================================================
 *  PRIVATE Tree remove methods
 */
static void __mtree_shrink (z_btree_t *self) {
  struct node *node;

  if (self->height <= 1)
    return;

  node = self->root;
  Z_ASSERT(node->nkeys < 1, "At least one key is needed");
  self->root = node->values[0];
  self->height--;

  __node_free(node);
}

static void *__mtree_remove_level (z_btree_t *self,
                                   unsigned int level,
                                   z_bytes_t *key);

static void __mnode_merge (z_btree_t *self, unsigned int level,
                           struct node *left,
                           struct node *right,
                           struct node *parent, unsigned int index)
{
  /* Move all keys to the left */
  z_memcpy(left->keys + left->nkeys,
           right->keys + 0,
           right->nkeys * sizeof(z_bytes_t *));
  z_memcpy(left->values + left->nkeys,
           right->values + 0,
           right->nkeys * sizeof(void *));
  left->nkeys += right->nkeys;

  /* Exchange left and right child in parent */
  parent->values[index] = right;
  parent->values[index + 1] = left;

  /* Remove left (formerly right) child from parent */
  __mtree_remove_level(self, level + 1, parent->keys[index]);
  __node_free(right);
}

static void __mtree_rebalance (z_btree_t *self,
                               unsigned int level,
                               z_bytes_t *key,
                               struct node *child)
{
  struct node *parent;
  unsigned int index;

  if (child->nkeys == 0) {
    __mtree_remove_level(self, level + 1, key);
    __node_free(child);
    return;
  }

  parent = __mtree_find_level(self, level + 1, key);
  index = __key_slot(parent->keys, parent->nkeys, key);
  Z_ASSERT(parent->values[index] == child, "");

  if (index > 0) {
    struct node *left;
    left = (struct node *)parent->values[index - 1];
    if ((child->nkeys + left->nkeys) <= __NITEMS) {
      __mnode_merge(self, level, left, child, parent, index - 1);
      return;
    }
  }

  if ((index + 1) < parent->nkeys) {
    struct node *right;
    right = (struct node *)parent->values[index + 1];
    if ((child->nkeys + right->nkeys) <= __NITEMS) {
      __mnode_merge(self, level, child, right, parent, index);
      return;
    }
  }
}

static void __mtree_update_parent (z_btree_t *self,
                                   z_bytes_t *key,
                                   z_bytes_t *new_key)
{
  unsigned int height = self->height;
  struct node *node = self->root;
  unsigned int index;

  for (; height > 1; height--) {
    index = __key_slot(node->keys, node->nkeys, key);
    if (index == __NITEMS)
      break;

    if (node->keys[index] != NULL && z_bytes_equals(node->keys[index], key)) {
      z_bytes_free(node->keys[index]);
      node->keys[index] = z_bytes_acquire(new_key);
    }

    if ((node = node->values[index]) == NULL)
      break;
  }
}

static void *__mtree_remove_level (z_btree_t *self,
                                   unsigned int level,
                                   z_bytes_t *key)
{
  unsigned int index;
  struct node *node;
  void *value;

  if (level > self->height) {
    self->height = 0;
    self->root = NULL;
    return(NULL);
  }

  node = __mtree_find_level(self, level, key);
  index = __key_slot(node->keys, node->nkeys, key);
  if (level == 1 && z_bytes_compare(node->keys[index], key))
    return(NULL);

  value = node->values[index];

  /* remove and shift */
  __node_remove(node, index);

  if (node->nkeys > 0 && index == node->nkeys ) {
    __mtree_update_parent(self, key, node->keys[node->nkeys - 1]);
    key = node->keys[node->nkeys - 1];
  }

  if (node->nkeys < (__NITEMS >> 1)) {
    if (level < self->height) {
      __mtree_rebalance(self, level, key, node);
    } else if (node->nkeys == 1) {
      __mtree_shrink(self);
    }
  }
  return(value);
}

/* ============================================================================
 *  PUBLIC tree
 */
int z_btree_open (z_btree_t *self) {
  self->root = NULL;
  self->height = 0;
  return(0);
}

void z_btree_close (z_btree_t *self) {
}

int z_btree_insert (z_btree_t *self, z_bytes_t *key, z_bytes_t *value) {
  __mtree_insert_level(self, 1, key, z_bytes_acquire(value));
  z_btree_debug(self);
  return(0);
}

z_bytes_t *z_btree_remove (z_btree_t *self, z_bytes_t *key) {
  Z_ASSERT(self->height > 0, "There should be at least one node");
  __mtree_remove_level(self, 1, key);
  z_btree_debug(self);
  return(0);
}

z_bytes_t *z_btree_lookup (const z_btree_t *btree, const z_bytes_t *key) {
  unsigned int height = btree->height;
  struct node *node = btree->root;
  unsigned int index;

  for (; height > 1; height--) {
    index = __key_slot(node->keys, node->nkeys, key);
    if (index == __NITEMS)
      return(NULL);

    if ((node = node->values[index]) == NULL)
      return(NULL);
  }

  index = __key_slot(node->keys, node->nkeys, key);
  if (index < node->nkeys && z_bytes_equals(node->keys[index], key))
    return(node->values[index]);
  return(NULL);
}

static void __debug (struct node *node, unsigned int height) {
  unsigned int i;

  fprintf(stderr, "Node %p %u: ", node, height);
  for (i = 0; i < node->nkeys; ++i) {
    char buffer[32];
    z_memcpy(buffer, z_bytes_data(node->keys[i]), z_bytes_size(node->keys[i]));
    buffer[z_bytes_size(node->keys[i])] = 0;
    fprintf(stderr, "%s -> ", buffer);
  }
  fprintf(stderr, "X\n");

  for (i = 0; i < node->nkeys; ++i) {
    if (height > 1) {
      __debug((struct node *)node->values[i], height - 1);
    }
  }
}

void z_btree_debug (const z_btree_t *self) {
  fprintf(stderr, "\nTree:\n");
  __debug((struct node *)self->root, self->height);
}