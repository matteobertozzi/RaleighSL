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

#include <string.h>

struct avl16_head {
  uint16_t stride;

  /* allocation */
  uint16_t next;
  uint16_t free_list;
  uint32_t avail;
  uint32_t size;

  /* uber */
  uint16_t root_current;
  uint16_t root_versions;
  uint16_t root_free;
} __attribute__((packed));

struct avl16_uber {
  uint64_t seqid : 48;
  uint64_t root  : 16;
} __attribute__((packed));

struct avl16_node {
  uint16_t child[2];
  int8_t   balance;
  uint32_t birth_hi;
  uint32_t death_hi;
  uint16_t birth_lo;
  uint16_t death_lo;
  uint8_t  key[1];
} __attribute__((packed));

/* ============================================================================
 *  PRIVATE AVL Macros
 */
#define __AVL_HEAD(x)             Z_CAST(struct avl16_head, x)

#define __AVL16_HEAD_SIZE         sizeof(struct avl16_head)
#define __AVL16_UBER_SIZE         sizeof(struct avl16_uber)
#define __AVL16_NODE_SIZE         (sizeof(struct avl16_node) - 1)

#define __AVL_BLK_NODE_DIFF(block, node)                                      \
  (((uint8_t *)(node)) - (block))

#define __AVL_STRIDE_BLK(stride, block, pos)                                  \
  ((block) + __AVL16_HEAD_SIZE + ((pos) - 1) * (stride))

#define __AVL_POS(stride, block, node)                                        \
  (1 + ((__AVL_BLK_NODE_DIFF(block, node) - __AVL16_HEAD_SIZE) / (stride)))

#define __AVL_NODE(stride, block, pos)                                        \
  Z_CAST(struct avl16_node, __AVL_STRIDE_BLK(stride, block, pos))

#define __AVL_UBER(eblock, index)                                             \
  Z_CAST(struct avl16_uber, (eblock) - ((index) * __AVL16_UBER_SIZE))

/* ============================================================================
 *  PRIVATE AVL Version Macros
 */
#define __u48_get(hi, lo)                                     \
  ((uint64_t)(hi) << 16 | (lo))

#define __u48_set(hi, lo, value)                              \
  do {                                                        \
    (hi) = ((value) >> 16) & 0xffffffff;                      \
    (lo) = (value) & 0xffff;                                  \
  } while (0)

#define __avl_node_birth(node)                                \
  __u48_get((node)->birth_hi, (node)->birth_lo)

#define __avl_node_set_birth(node, value)                     \
  __u48_set((node)->birth_hi, (node)->birth_lo, value)

#define __avl_node_death(node)                                \
  __u48_get((node)->death_hi, (node)->death_lo)

#define __avl_node_set_death(node, value)                     \
  __u48_set((node)->death_hi, (node)->death_lo, value)

/* ============================================================================
 *  PRIVATE AVL uber
 */
static struct avl16_uber *__avl16_add_root (uint8_t *block,
                                            uint64_t seqid,
                                            uint16_t root)
{
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_uber *uber;

  if (head->root_free != 0) {
    uber = __AVL_UBER(block + head->size, head->root_free);
    head->root_current = head->root_free;
    head->root_free = uber->root;
  } else {
    head->root_versions += 1;
    head->root_current = head->root_versions;
    head->avail -= __AVL16_UBER_SIZE;
    uber = __AVL_UBER(block + head->size, head->root_current);
  }
  uber->seqid = seqid;
  uber->root  = root;
  return(uber);
}

 static void __avl16_remove_root (uint8_t *block,
                                  struct avl16_uber *uber,
                                  uint16_t index)
 {
  struct avl16_head *head = __AVL_HEAD(block);
  uber->seqid = 0;
  if (0 && head->root_versions == index) {
    uber->root = 0;
    head->root_versions -= 1;
    head->avail += __AVL16_UBER_SIZE;
  } else {
    uber->root = head->root_free;
    head->root_free = index;
  }
}

static struct avl16_uber *__avl16_find_root (uint8_t *block, uint64_t seqid) {
  const struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_uber *uber;
  int count;
  count = head->root_versions;
  uber = __AVL_UBER(block + head->size, count);
  while (count--) {
    if (uber->seqid == seqid)
      return(uber);
    uber++;
  }
  return(NULL);
}

/* ============================================================================
 *  PRIVATE COW
 */
static uint16_t __alloc_node (struct avl16_head *head, uint8_t *block) {
  uint16_t node_pos = head->next;
  if (head->free_list == 0) {
    head->next += 1;
  } else {
    const struct avl16_node *node;
    node_pos = head->free_list;
    node = __AVL_NODE(head->stride, block, node_pos);
    head->free_list = node->child[0];
  }
  head->avail -= head->stride;
  return node_pos;
}

static void __free_node (struct avl16_head *head,
                         struct avl16_node *node,
                         uint16_t node_pos)
{
  node->child[0] = head->free_list;
  node->child[1] = 0;
  node->balance = 0;
  __avl_node_set_birth(node, 0);
  __avl_node_set_death(node, 0xffffffffffff);
  head->free_list = node_pos;
  head->avail += head->stride;
}

#define __avl16_copy_node(dst, src, seqid, stride)              \
  do {                                                          \
    memcpy(dst, src, stride);                                   \
    __avl_node_set_birth(dst, seqid);                           \
    __avl_node_set_death(dst, 0xffffffffffff);                  \
    __avl_node_set_death(src, seqid);                           \
  } while (0)

static void __avl16cow_copy (uint8_t *block,
                             uint64_t seqid,
                             uint16_t *root,
                             struct avl16_node *stack[20],
                             int istack,
                             uint32_t dstack)
{
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_node *parent;
  int i;

  if (seqid != __avl_node_birth(stack[0])) {
    *root = __alloc_node(head, block);
    parent = __AVL_NODE(head->stride, block, *root);
    __avl16_copy_node(parent, stack[0], seqid, head->stride);
    stack[0] = parent;
  } else {
    parent = stack[0];
  }

  for (i = 1; i < istack; ++i) {
    if (seqid != __avl_node_birth(stack[i])) {
      struct avl16_node *node;
      uint16_t node_pos;

      node_pos = __alloc_node(head, block);
      parent->child[dstack & 1] = node_pos;

      node = __AVL_NODE(head->stride, block, node_pos);
      __avl16_copy_node(node, stack[i], seqid, head->stride);
      stack[i] = node;
      parent = node;
    } else {
      parent = stack[i];
    }
    dstack >>= 1;
  }
}

static struct avl16_node *__avl16_cow (struct avl16_head *head,
                                       uint8_t *block,
                                       uint64_t seqid,
                                       uint16_t stride,
                                       uint16_t *node_pos)
{
  struct avl16_node *node;
  if (*node_pos == 0)
    return(NULL);
  node = __AVL_NODE(stride, block, *node_pos);
  if (seqid != __avl_node_birth(node)) {
    struct avl16_node *cow_node;
    *node_pos = __alloc_node(head, block);
    cow_node = __AVL_NODE(stride, block, *node_pos);
    __avl16_copy_node(cow_node, node, seqid, stride);
    return(cow_node);
  }
  return(node);
}

/* ============================================================================
 *  PRIVATE AVL Insert/Balance
 */
static void __avl16cow_ibalance (uint8_t *block,
                                 uint64_t seqid,
                                 struct avl16_node *parent,
                                 struct avl16_node *node,
                                 uint16_t *top,
                                 uint32_t dstack)
{
  struct avl16_head *head = __AVL_HEAD(block);
  const uint16_t stride = head->stride;
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
    Z_ASSERT(seqid == __avl_node_birth(x), "Invalid birth");

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
      Z_ASSERT(seqid == __avl_node_birth(w), "Invalid birth");

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
    Z_ASSERT(seqid == __avl_node_birth(x), "Invalid birth");

    if (x->balance == +1) {
      wpos = xpos;
      parent->child[1] = x->child[0];
      x->child[0] = parent_pos;
      x->balance  = parent->balance = 0;
    } else {
      struct avl16_node *w;

      wpos = x->child[0];
      w = __AVL_NODE(stride, block, wpos);
      Z_ASSERT(seqid == __avl_node_birth(w), "Invalid birth");

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
static void __avl16cow_dbalance (uint8_t *block,
                                 uint64_t seqid,
                                 const struct avl16_node *dnode,
                                 struct avl16_node *stack[20],
                                 uint8_t dstack[20],
                                 int k)
{
  struct avl16_head *head = __AVL_HEAD(block);
  const uint16_t stride = head->stride;

  if (dnode->child[1] == 0) {
    stack[k - 1]->child[dstack[k - 1]] = dnode->child[0];
  } else {
    struct avl16_node node;
    struct avl16_node *r;
    uint16_t rpos;

    node.child[0] = dnode->child[0];
    node.child[1] = dnode->child[1];
    node.balance  = dnode->balance;

    r = __avl16_cow(head, block, seqid, stride, &(node.child[1]));
    rpos = node.child[1];

    if (r->child[0] == 0) {
      r->child[0] = node.child[0];
      r->balance  = node.balance;
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
        s = __avl16_cow(head, block, seqid, stride, &(r->child[0]));
        spos = r->child[0];
        if (s->child[0] == 0)
          break;

        r = s;
      }

      s->child[0] = node.child[0];
      r->child[0] = s->child[1];
      s->child[1] = node.child[1];
      s->balance  = node.balance;

      stack[j - 1]->child[dstack[j - 1]] = spos;
      dstack[j] = 1;
      stack[j]  = s;
    }
  }

  while (--k > 0) {
    struct avl16_node *y = stack[k];
    const uint16_t ypos = __AVL_POS(stride, block, y);
    Z_ASSERT(seqid == __avl_node_birth(y), "Invalid birth");

    if (dstack[k] == 0) {
      ++(y->balance);
      if (y->balance == +1)
        break;
      if (y->balance == +2) {
        struct avl16_node *x;
        uint16_t xpos;

        x = __avl16_cow(head, block, seqid, stride, &(y->child[1]));
        xpos = y->child[1];

        if (x->balance == -1) {
          struct avl16_node *w;
          uint16_t wpos;

          w = __avl16_cow(head, block, seqid, stride, &(x->child[0]));
          wpos = x->child[0];

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
        struct avl16_node *x;
        uint16_t xpos;

        x = __avl16_cow(head, block, seqid, stride, &(y->child[0]));
        xpos = y->child[0];

        if (x->balance == +1) {
          struct avl16_node *w;
          uint16_t wpos;

          w = __avl16_cow(head, block, seqid, stride, &(x->child[1]));
          wpos = x->child[1];

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

/* ============================================================================
 *  PRIVATE node
 */
static void __avl16_clean (uint8_t *block, uint64_t seqid, uint16_t node_pos) {
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_node *node;

  node = __AVL_NODE(head->stride, block, node_pos);
  if ((seqid + 1) < __avl_node_death(node)) {
    return;
  }

  if (node->child[0] != 0)
    __avl16_clean(block, seqid, node->child[0]);

  if (node->child[1] != 0)
    __avl16_clean(block, seqid, node->child[1]);

  __free_node(head, node, node_pos);
}

static void *__avl16_append (uint8_t *block,
                             uint64_t seqid,
                             uint16_t *root)
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
  __avl_node_set_birth(node, seqid);
  __avl_node_set_death(node, 0xffffffffffff);

  if (*root != 0) {
    struct avl16_node *stack[20];
    uint32_t dfstack;
    uint32_t dstack;
    uint16_t pp;
    int parent;
    int istack;
    int q, top;
    uint8_t tr;
    int k;

    k = 0;
    tr = 1;
    parent = 0;
    istack = 0;
    dstack = 0;
    dfstack = 0;
    q = top = 0;
    pp = *root;
    while (pp != 0) {
      struct avl16_node *p = __AVL_NODE(head->stride, block, pp);
      if (p->balance != 0) {
        k = 0;
        top = q;
        tr = !istack;
        parent = istack;
        dstack = 0;
      }

      q = istack;
      dfstack |= (1 << istack);
      stack[istack++] = p;
      pp = p->child[1];
      dstack |= (1 << k++);
    }

    if (head->avail < ((istack + 2) * head->stride))
      return(NULL);
    __avl16cow_copy(block, seqid, root, stack, istack, dfstack);

    stack[q]->child[1] = node_pos;
    if (tr) {
      __avl16cow_ibalance(block, seqid, stack[parent], node, root, dstack);
    } else {
      __avl16cow_ibalance(block, seqid, stack[parent], node, stack[top]->child, dstack);
    }
  } else {
    *root = node_pos;
  }
  return node->key;
}


static void *__avl16_insert (uint8_t *block,
                             uint64_t seqid,
                             uint16_t *root,
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
  __avl_node_set_birth(node, seqid);
  __avl_node_set_death(node, 0xffffffffffff);

  /* lookup the insertion point */
  if (*root != 0) {
    struct avl16_node *stack[20];
    uint32_t dfstack;
    uint32_t dstack;
    uint16_t pp;
    int parent;
    int istack;
    int q, top;
    int k, dir;
    uint8_t tr;

    tr = 1;
    parent = 0;
    istack = 0;
    dstack = 0;
    dfstack = 0;
    k = dir = 0;
    q = top = 0;
    pp = *root;
    while (pp != 0) {
      struct avl16_node *p = __AVL_NODE(head->stride, block, pp);
      const int cmp = key_cmp(udata, p->key, key);
      if (Z_UNLIKELY(cmp == 0)) {
        __free_node(head, node, node_pos);
        return(p->key);
      }

      if (p->balance != 0) {
        k = 0;
        top = q;
        tr = !istack;
        parent = istack;
        dstack = 0;
      }

      q = istack;
      dir = cmp < 0;
      dstack |= (dir << k++);
      dfstack |= (dir << istack);
      stack[istack++] = p;
      pp = p->child[dir];
    }

    if (head->avail < ((istack + 2) * head->stride)) {
      printf("NO SPACE %u istack=%u %u\n", head->avail, istack, (istack + 2) * head->stride);
      return(NULL);
    }
    __avl16cow_copy(block, seqid, root, stack, istack, dfstack);

    if (istack > 0) {
      stack[q]->child[dir] = node_pos;
    } else {
      *root = node_pos;
    }

    if (tr) {
      __avl16cow_ibalance(block, seqid, stack[parent], node, root, dstack);
    } else {
      __avl16cow_ibalance(block, seqid, stack[parent], node, stack[top]->child, dstack);
    }
  } else {
    *root = node_pos;
  }
  return node->key;
}

static void *__avl16_remove (uint8_t *block,
                             uint64_t seqid,
                             uint16_t *root,
                             z_compare_t key_cmp,
                             const void *key,
                             void *udata)
{
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_node *stack[20];
  struct avl16_node *node;
  uint8_t dstack[20];
  uint16_t node_pos;
  uint32_t dfstack;
  int istack;
  int cmp;

  if (*root == 0)
    return(NULL);

  cmp = 1;
  istack = 0;
  dfstack = 0;
  node = (struct avl16_node *)root;
  do {
    const int dir = cmp < 0;
    dfstack |= (dir << istack);

    stack[istack] = node;
    dstack[istack++] = dir;

    if (node->child[dir] == 0)
      return(NULL);

    node_pos = node->child[dir];
    node = __AVL_NODE(head->stride, block, node_pos);
  } while ((cmp = key_cmp(udata, node->key, key)) != 0);

  if (head->avail < ((2 + istack) * head->stride))
    return(NULL);

  if (istack > 1) {
    __avl16cow_copy(block, seqid, root, stack + 1, istack - 1, dfstack >> 1);
  }

  __avl16cow_dbalance(block, seqid, node, stack, dstack, istack);
  __avl_node_set_death(node, seqid);
  return(node->key);
}

/* ============================================================================
 *  PUBLIC API
 */
uint32_t z_avl16cow_init (uint8_t *block, uint32_t size, uint16_t stride) {
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_uber *uber;

  head->stride     = __AVL16_NODE_SIZE + stride;

  head->next       = 1;
  head->free_list  = 0;
  head->avail      = size - __AVL16_HEAD_SIZE - __AVL16_UBER_SIZE;
  head->size       = size;

  /* uber */
  head->root_current  = 1;
  head->root_versions = 1;
  head->root_free     = 0;

  uber = __AVL_UBER(block + size, 1);
  uber->seqid = 1;
  uber->root  = 0;
  return((head->avail / head->stride) - 1);
}

int z_avl16cow_clean (uint8_t *block, uint64_t seqid) {
  const struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_uber *uber;
  uint16_t i, count;
  count = head->root_versions;
  uber = __AVL_UBER(block + head->size, count);
  for (i = 0; i < count; ++i) {
    if (uber->seqid <= seqid && uber->seqid > 0) {
      if (uber->root != 0)
        __avl16_clean(block, seqid, uber->root);
      __avl16_remove_root(block, uber, count - i);
    }
    uber++;
  }
  return(0);
}

int z_avl16cow_clean_all (uint8_t *block) {
  const struct avl16_head *head = __AVL_HEAD(block);
  const struct avl16_uber *uber;
  uber = __AVL_UBER(block + head->size, head->root_current);
  return(z_avl16cow_clean(block, uber->seqid - 1));
}

/* ============================================================================
 *  PUBLIC API Transaction
 */
int z_avl16_txn_open (z_avl16_txn_t *self, uint8_t *block, uint64_t seqid) {
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_uber *uber;

  if (seqid == 0) {
    uber = __AVL_UBER(block + head->size, head->root_current);
  } else {
    uber = __avl16_find_root(block, seqid);
    if (uber == NULL)
      return(1);
  }

  self->seqid  = uber->seqid;
  self->root   = uber->root;
  self->dirty  = 0;
  self->failed = 0;
  return(0);
}

static void __avl16_revert (uint8_t *block, uint64_t seqid, uint16_t node_pos) {
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_node *node;

  node = __AVL_NODE(head->stride, block, node_pos);
  if (seqid == __avl_node_death(node)) {
    __avl_node_set_death(node, 0xffffffffffff);
  }

  if (seqid != __avl_node_birth(node)) {
    return;
  }

  if (node->child[0] != 0)
    __avl16_revert(block, seqid, node->child[0]);

  if (node->child[1] != 0)
    __avl16_revert(block, seqid, node->child[1]);

  __free_node(head, node, node_pos);
}

static void __avl16_revert_death (uint8_t *block, uint64_t seqid, uint16_t node_pos) {
  struct avl16_head *head = __AVL_HEAD(block);
  struct avl16_node *node;
  uint64_t death;

  node = __AVL_NODE(head->stride, block, node_pos);
  death = __avl_node_death(node);
  if (seqid > death)
    return;

  if (seqid == death) {
    __avl_node_set_death(node, 0xffffffffffff);
  }

  if (node->child[0] != 0)
    __avl16_revert_death(block, seqid, node->child[0]);

  if (node->child[1] != 0)
    __avl16_revert_death(block, seqid, node->child[1]);
}

void z_avl16_txn_revert (z_avl16_txn_t *self, uint8_t *block) {
  if (self->dirty) {
    const struct avl16_head *head = __AVL_HEAD(block);
    struct avl16_uber *uber;

    if (self->root != 0) {
      __avl16_revert(block, self->seqid, self->root);
    }

    uber = __AVL_UBER(block + head->size, head->root_current);
    if (uber->root != 0)
      __avl16_revert_death(block, self->seqid, uber->root);
    self->seqid  = uber->seqid;
    self->root   = uber->root;
    self->dirty  = 0;
    self->failed = 0;
  }
}

int z_avl16_txn_commit (z_avl16_txn_t *self, uint8_t *block) {
  if (self->failed) {
    z_avl16_txn_revert(self, block);
    return(1);
  }
  if (self->dirty) {
    struct avl16_head *head = __AVL_HEAD(block);
    if (head->avail < __AVL16_UBER_SIZE) {
      z_avl16_txn_revert(self, block);
      return(2);
    }
    __avl16_add_root(block, self->seqid, self->root);
  }
  return(0);
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
  printf("%03u[%llu:%llx:%03u/",
         root,
         __avl_node_birth(node),
         __avl_node_death(node),
         node->child[0]);
  __dump_bytes(node->key, 4);
  printf("/%03u] ", node->child[1]);
  __avl_dump(block, node->child[1]);
}

void z_avl16_txn_dump (z_avl16_txn_t *self, uint8_t *block) {
  const struct avl16_head *head = (const struct avl16_head *)block;
  int i;
  for (i = 1; i <= head->root_versions; ++i) {
    struct avl16_uber *uber = __AVL_UBER(block + head->size, i);
    printf("%d[%llu:%u] -> ", i, uber->seqid, uber->root);
  }
  printf("[TXN %llu:%u]\n", self->seqid, self->root);

  for (i = 1; i <= head->root_versions; ++i) {
    struct avl16_uber *uber = __AVL_UBER(block + head->size, i);
    printf("HEAD %d seqid=%-4llu root=%-2u next=%-2u avail=%-5u: ",
           i, uber->seqid, uber->root, head->next, head->avail);
    __avl_dump(block, uber->root);
    printf("\n");
  }

  {
    printf("TXN-HEAD %d seqid=%-4llu root=%-2u: ",
           i, self->seqid, self->root);
    __avl_dump(block, self->root);
    printf("\n");
  }
}

void *z_avl16_txn_append (z_avl16_txn_t *self, uint8_t *block) {
  void *ptr;
  if (!self->dirty) {
    self->dirty = 1;
    self->seqid++;
  }
  ptr = __avl16_append(block, self->seqid, &(self->root));
  self->failed |= (ptr == NULL);
  return(ptr);
}

void *z_avl16_txn_insert (z_avl16_txn_t *self,
                          uint8_t *block,
                          z_compare_t key_cmp,
                          const void *key,
                          void *udata)
{
  void *ptr;
  if (!self->dirty) {
    self->dirty = 1;
    self->seqid++;
  }
  ptr = __avl16_insert(block, self->seqid, &(self->root), key_cmp, key, udata);
  self->failed |= (ptr == NULL);
  return(ptr);
}

void *z_avl16_txn_remove (z_avl16_txn_t *self,
                          uint8_t *block,
                          z_compare_t key_cmp,
                          const void *key,
                          void *udata)
{
  void *ptr;
  if (!self->dirty) {
    self->dirty = 1;
    self->seqid++;
  }
  ptr = __avl16_remove(block, self->seqid, &(self->root), key_cmp, key, udata);
  self->failed |= (ptr == NULL);
  return(ptr);
}

void *z_avl16_txn_lookup (z_avl16_txn_t *self,
                          uint8_t *block,
                          z_compare_t key_cmp,
                          const void *key,
                          void *udata)
{
  const uint16_t stride = __AVL_HEAD(block)->stride;
  uint16_t root = self->root;
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
