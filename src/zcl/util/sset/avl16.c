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

#include <zcl/debug.h>
#include <zcl/avl.h>

struct avl16_head {
  uint16_t stride;
  uint16_t root;
  uint16_t next;
  uint16_t free_list;
} __attribute__((packed));

struct avl16_node {
  uint16_t child[2];
  int8_t   balance;
  uint8_t  key[1];
} __attribute__((packed));

/* ============================================================================
 *  PRIVATE AVL Macros
 */
#define __AVL_HEAD(x)             Z_CAST(struct avl16_head, x)

#define __AVL16_HEAD_SIZE         sizeof(struct avl16_head)
#define __AVL16_NODE_SIZE         (sizeof(struct avl16_node) - 1)

#define __AVL_BLK_NODE_DIFF(block, node)                                      \
  (((uint8_t *)(node)) - (block))

#define __AVL_STRIDE_BLK(stride, block, pos)                                  \
  ((block) + __AVL16_HEAD_SIZE + ((pos) - 1) * (stride))

#define __AVL_POS(stride, block, node)                                        \
  (1 + ((__AVL_BLK_NODE_DIFF(block, node) - __AVL16_HEAD_SIZE) / (stride)))

#define __AVL_NODE(stride, block, pos)                                        \
  Z_CAST(struct avl16_node, __AVL_STRIDE_BLK(stride, block, pos))

/* ============================================================================
 *  PRIVATE AVL Insert/Balance
 */
static void __avl16_ibalance (uint8_t *block,
                              struct avl16_node *parent,
                              struct avl16_node *node,
                              uint16_t *top,
                              uint32_t dstack)
{
  const uint16_t stride = __AVL_HEAD(block)->stride;
  const uint16_t parent_pos = __AVL_POS(stride, block, parent);
  uint16_t wpos;

  do {
    struct avl16_node *p = parent;
    while (p != node) {
      const int dir = dstack & 1;
      p->balance += (dir ? 1 : -1);
      p = __AVL_NODE(stride, block, p->child[dir]);
      dstack >>= 1;
    }
  } while (0);

  if (parent->balance == -2) {
    const uint16_t xpos = parent->child[0];
    struct avl16_node *x = __AVL_NODE(stride, block, xpos);
    if (x->balance == -1) {
      wpos = xpos;
      parent->child[0] = x->child[1];
      parent->balance  = 0;
      x->child[1] = parent_pos;
      x->balance  = 0;
    } else {
      struct avl16_node *w;
      wpos = x->child[1];
      w = __AVL_NODE(stride, block, wpos);
      x->child[1] = w->child[0];
      w->child[0] = xpos;
      parent->child[0] = w->child[1];
      w->child[1] = parent_pos;
      if (w->balance == -1) {
        x->balance = 0;
        parent->balance = +1;
      } else if (w->balance == 0) {
        x->balance = 0;
        parent->balance = 0;
      } else /* |w->balance == +1| */ {
        x->balance = -1;
        parent->balance = 0;
      }
      w->balance = 0;
    }
  } else if (parent->balance == +2) {
    const uint16_t xpos = parent->child[1];
    struct avl16_node *x = __AVL_NODE(stride, block, xpos);
    if (x->balance == +1) {
      wpos = xpos;
      parent->child[1] = x->child[0];
      x->child[0] = parent_pos;
      x->balance  = parent->balance = 0;
    } else {
      struct avl16_node *w;
      wpos = x->child[0];
      w = __AVL_NODE(stride, block, wpos);
      x->child[0] = w->child[1];
      w->child[1] = xpos;
      parent->child[1] = w->child[0];
      w->child[0] = parent_pos;
      if (w->balance == +1) {
        x->balance = 0;
        parent->balance = -1;
      } else if (w->balance == 0) {
        x->balance = 0;
        parent->balance = 0;
      } else /* |w->balance == -1| */ {
        x->balance = +1;
        parent->balance = 0;
      }
      w->balance = 0;
    }
  } else {
    return;
  }

  top[parent_pos != top[0]] = wpos;
}

/* ============================================================================
 *  PRIVATE AVL Delete/Balance
 */
static void __avl16_dbalance (uint8_t *block,
                              const struct avl16_node *node,
                              struct avl16_node *stack[20],
                              uint8_t dstack[20],
                              int k)
{
  const uint16_t stride = __AVL_HEAD(block)->stride;

  if (node->child[1] == 0) {
    stack[k - 1]->child[dstack[k - 1]] = node->child[0];
  } else {
    uint16_t rpos = node->child[1];
    struct avl16_node *r = __AVL_NODE(stride, block, rpos);
    if (r->child[0] == 0) {
      r->child[0] = node->child[0];
      r->balance  = node->balance;
      stack[k - 1]->child[dstack[k - 1]] = rpos;
      dstack[k] = 1;
      stack[k++] = r;
    } else {
      struct avl16_node *s;
      uint16_t spos;
      int j = k++;

      while (1) {
        dstack[k]  = 0;
        stack[k++] = r;
        spos = r->child[0];
        s = __AVL_NODE(stride, block, spos);
        if (s->child[0] == 0)
          break;

        r = s;
      }

      s->child[0] = node->child[0];
      r->child[0] = s->child[1];
      s->child[1] = node->child[1];
      s->balance  = node->balance;

      stack[j - 1]->child[dstack[j - 1]] = spos;
      dstack[j] = 1;
      stack[j]  = s;
    }
  }

  while (--k > 0) {
    struct avl16_node *y = stack[k];
    const uint16_t ypos = __AVL_POS(stride, block, y);

    if (dstack[k] == 0) {
      ++(y->balance);
      if (y->balance == +1)
        break;
      if (y->balance == +2) {
        const uint16_t xpos = y->child[1];
        struct avl16_node *x = __AVL_NODE(stride, block, xpos);
        if (x->balance == -1) {
          const uint16_t wpos = x->child[0];
          struct avl16_node *w;
          w = __AVL_NODE(stride, block, wpos);
          x->child[0] = w->child[1];
          w->child[1] = xpos;
          y->child[1] = w->child[0];
          w->child[0] = ypos;
          if (w->balance == +1) {
            x->balance = 0;
            y->balance = -1;
          } else if (w->balance == 0) {
            x->balance = 0;
            y->balance = 0;
          } else /* |w->balance == -1| */ {
            x->balance = +1;
            y->balance = 0;
          }
          w->balance = 0;
          stack[k - 1]->child[dstack[k - 1]] = wpos;
        } else {
          y->child[1] = x->child[0];
          x->child[0] = ypos;
          stack[k - 1]->child[dstack[k - 1]] = xpos;
          if (x->balance == 0) {
            x->balance = -1;
            y->balance = +1;
            break;
          } else {
            x->balance = 0;
            y->balance = 0;
          }
        }
      }
    } else {
      --(y->balance);
      if (y->balance == -1)
        break;
      if (y->balance == -2) {
        const uint16_t xpos = y->child[0];
        struct avl16_node *x = __AVL_NODE(stride, block, xpos);
        if (x->balance == +1) {
          const uint16_t wpos = x->child[1];
          struct avl16_node *w;
          w = __AVL_NODE(stride, block, wpos);
          x->child[1] = w->child[0];
          w->child[0] = xpos;
          y->child[0] = w->child[1];
          w->child[1] = ypos;
          if (w->balance == -1) {
            x->balance = 0;
            y->balance = +1;
          } else if (w->balance == 0) {
            x->balance = 0;
            y->balance = 0;
          } else /* |w->balance == +1| */ {
            x->balance = -1;
            y->balance = 0;
          }
          w->balance = 0;
          stack[k - 1]->child[dstack[k - 1]] = wpos;
        } else {
          y->child[0] = x->child[1];
          x->child[1] = ypos;
          stack[k - 1]->child[dstack[k - 1]] = xpos;
          if (x->balance == 0) {
            x->balance = +1;
            y->balance = -1;
            break;
          } else {
            x->balance = 0;
            y->balance = 0;
          }
        }
      }
    }
  }
}

static uint16_t __alloc_node (struct avl16_head *head, uint8_t *block) {
  uint16_t node_pos = head->next;
  if (head->stride > 1) {
    if (head->free_list == 0) {
      head->next += 1;
    } else {
      const struct avl16_node *node;
      node_pos = head->free_list;
      node = __AVL_NODE(head->stride, block, node_pos);
      head->free_list = node->child[0];
    }
  } else {
    head->next += __AVL16_NODE_SIZE;
  }
  return node_pos;
}

/* ============================================================================
 *  PUBLIC API
 */
uint32_t z_avl16_init (uint8_t *block, uint32_t size, uint16_t stride) {
  struct avl16_head *head = __AVL_HEAD(block);
  head->stride    = (stride > 0) ? (__AVL16_NODE_SIZE + stride) : 1;
  head->root      = 0;
  head->next      = 1;
  head->free_list = 0;
  return((size - __AVL16_HEAD_SIZE) / head->stride);
}

void z_avl16_expand (uint8_t *block, uint32_t size) {
  struct avl16_head *head = __AVL_HEAD(block);
  Z_ASSERT(head->stride == 1, "Expected no-stride got %u", head->stride);
  head->next += size;
}

uint32_t z_avl16_avail (const uint8_t *block) {
  struct avl16_head *head = __AVL_HEAD(block);
  return 0xffff - head->next;
}

void *z_avl16_append (uint8_t *block) {
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_node *node;
  uint16_t node_pos;

  /* prepare new node */
  node_pos = __alloc_node(head, block);
  node = __AVL_NODE(head->stride, block, node_pos);
  node->child[0] = 0;
  node->child[1] = 0;
  node->balance  = 0;

  if (head->root != 0) {
    struct avl16_node *parent;
    uint32_t dstack;
    uint16_t *top;
    uint16_t *q;
    uint16_t pp;
    int k;

    k = 0;
    dstack = 0;
    pp = head->root;
    top = q = &(head->root);
    parent = __AVL_NODE(head->stride, block, pp);
    while (pp != 0) {
      struct avl16_node *p = __AVL_NODE(head->stride, block, pp);
      if (p->balance != 0) {
        k = 0;
        top = q;
        parent = p;
        dstack = 0;
      }

      q = p->child;
      pp = p->child[1];
      dstack |= (1 << k++);
    }

    q[1] = node_pos;
    __avl16_ibalance(block, parent, node, top, dstack);
  } else {
    head->root = node_pos;
  }
  return node->key;
}

void *z_avl16_insert (uint8_t *block,
                      z_compare_t key_cmp,
                      const void *key,
                      void *udata)
{
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_node *node;
  uint16_t node_pos;

  /* prepare new node */
  node_pos = __alloc_node(head, block);
  node = __AVL_NODE(head->stride, block, node_pos);
  node->child[0] = 0;
  node->child[1] = 0;
  node->balance  = 0;

  /* lookup the insertion point */
  if (head->root != 0) {
    struct avl16_node *parent;
    uint32_t dstack;
    uint16_t *top;
    uint16_t *q;
    uint16_t pp;
    int k, dir;

    dstack = 0;
    k = dir = 0;
    pp = head->root;
    top = q = &(head->root);
    parent = __AVL_NODE(head->stride, block, pp);
    while (pp != 0) {
      struct avl16_node *p = __AVL_NODE(head->stride, block, pp);
      const int cmp = key_cmp(udata, p->key, key);
      if (Z_UNLIKELY(cmp == 0)) {
        if (head->stride > 1) {
          node->child[0] = head->free_list;
          head->free_list = node_pos;
        } else {
          head->next -= __AVL16_NODE_SIZE;
        }
        return(p->key);
      }

      if (p->balance != 0) {
        k = 0;
        top = q;
        parent = p;
        dstack = 0;
      }

      dir = cmp < 0;
      q  = p->child;
      pp = p->child[dir];
      dstack |= (dir << k++);
    }

    q[dir] = node_pos;
    __avl16_ibalance(block, parent, node, top, dstack);
  } else {
    head->root = node_pos;
  }
  return node->key;
}

void *z_avl16_remove (uint8_t *block,
                      z_compare_t key_cmp,
                      const void *key,
                      void *udata)
{
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_node *stack[20];
  struct avl16_node *node;
  uint8_t dstack[20];
  uint16_t node_pos;
  int istack;
  int cmp;

  if (head->root == 0)
    return(NULL);

  cmp = 1;
  istack = 0;
  node = (struct avl16_node *)(&(head->root));
  do {
    const int dir = cmp < 0;

    stack[istack] = node;
    dstack[istack++] = dir;
    if (node->child[dir] == 0)
      return(NULL);

    node_pos = node->child[dir];
    node = __AVL_NODE(head->stride, block, node_pos);
  } while ((cmp = key_cmp(udata, node->key, key)) != 0);

  __avl16_dbalance(block, node, stack, dstack, istack);
  node->child[0] = head->free_list;
  node->child[1] = 0;

  head->free_list = node_pos;
  return(node->key);
}

void *z_avl16_lookup (uint8_t *block,
                      z_compare_t key_cmp,
                      const void *key,
                      void *udata)
{
  const struct avl16_head *head = __AVL_HEAD(block);
  const uint16_t stride = head->stride;
  uint16_t root;

  root = head->root;
  while (root != 0) {
    struct avl16_node *node = __AVL_NODE(stride, block, root);
    const int cmp = key_cmp(udata, key, node->key);
    if (cmp < 0) {
      root = node->child[0];
    } else if (cmp > 0) {
      root = node->child[1];
    } else {
      return(node->key);
    }
  }
  return(NULL);
}

static void __dump_bytes (const void *bytes, size_t length) {
  const uint8_t *p = (const uint8_t *)bytes;
  while (length--)
    printf("%02x", *p++);
}

static void __avl_dump (const uint8_t *block, uint16_t root) {
  const struct avl16_head *head = (const struct avl16_head *)block;
  const struct avl16_node *node;
  if (root == 0)
    return;

  node = __AVL_NODE(head->stride, block, root);
  __avl_dump(block, node->child[0]);
  printf("%03u[%03u/", root, node->child[0]);
  __dump_bytes(node->key, 4);
  printf("/%03u] ", node->child[1]);
  __avl_dump(block, node->child[1]);
}

void z_avl16_dump (const uint8_t *block) {
  const struct avl16_head *head = (const struct avl16_head *)block;
  printf("HEAD root=%u next=%u: ", head->root, head->next);
  __avl_dump(block, head->root);
  printf("\n");
}
