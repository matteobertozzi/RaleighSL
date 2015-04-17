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

#include <zcl/avl16.h>

#define Z_ASSERT(x, ...) while (0)

/* ============================================================================
 *  PRIVATE AVL Macros
 */
#define __avl16_node_init(node)               \
  do {                                        \
    (node)->child[0] = 0;                     \
    (node)->child[1] = 0;                     \
    (node)->balance  = 0;                     \
  } while (0)

/* ============================================================================
 *  PRIVATE AVL Insert/Balance
 */
static void __avl16_ibalance (uint8_t *block,
                              z_avl16_node_t *parent,
                              z_avl16_node_t *node,
                              uint16_t *top,
                              uint32_t dstack)
{
  const uint16_t parent_pos = Z_AVL16_POS(block, parent);
  uint16_t wpos;

  do {
    z_avl16_node_t *p = parent;
    while (p != node) {
      const int dir = dstack & 1;
      p->balance += (dir ? 1 : -1);
      p = Z_AVL16_NODE(block, p->child[dir]);
      dstack >>= 1;
    }
  } while (0);

  if (parent->balance == -2) {
    const uint16_t xpos = parent->child[0];
    z_avl16_node_t *x = Z_AVL16_NODE(block, xpos);
    if (x->balance == -1) {
      wpos = xpos;
      parent->child[0] = x->child[1];
      parent->balance  = 0;
      x->child[1] = parent_pos;
      x->balance  = 0;
    } else {
      z_avl16_node_t *w;
      wpos = x->child[1];
      w = Z_AVL16_NODE(block, wpos);
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
    z_avl16_node_t *x = Z_AVL16_NODE(block, xpos);
    if (x->balance == +1) {
      wpos = xpos;
      parent->child[1] = x->child[0];
      x->child[0] = parent_pos;
      x->balance  = parent->balance = 0;
    } else {
      z_avl16_node_t *w;
      wpos = x->child[0];
      w = Z_AVL16_NODE(block, wpos);
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
                              const z_avl16_node_t *node,
                              z_avl16_node_t *stack[20],
                              uint8_t dstack[20],
                              int k)
{
  if (node->child[1] == 0) {
    stack[k - 1]->child[dstack[k - 1]] = node->child[0];
  } else {
    uint16_t rpos = node->child[1];
    z_avl16_node_t *r = Z_AVL16_NODE(block, rpos);
    if (r->child[0] == 0) {
      r->child[0] = node->child[0];
      r->balance  = node->balance;
      stack[k - 1]->child[dstack[k - 1]] = rpos;
      dstack[k] = 1;
      stack[k++] = r;
    } else {
      z_avl16_node_t *s;
      uint16_t spos;
      int j = k++;

      while (1) {
        dstack[k]  = 0;
        stack[k++] = r;
        spos = r->child[0];
        s = Z_AVL16_NODE(block, spos);
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
    z_avl16_node_t *y = stack[k];
    const uint16_t ypos = Z_AVL16_POS(block, y);

    if (dstack[k] == 0) {
      ++(y->balance);
      if (y->balance == +1)
        break;
      if (y->balance == +2) {
        const uint16_t xpos = y->child[1];
        z_avl16_node_t *x = Z_AVL16_NODE(block, xpos);
        if (x->balance == -1) {
          const uint16_t wpos = x->child[0];
          z_avl16_node_t *w = Z_AVL16_NODE(block, wpos);
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
        z_avl16_node_t *x = Z_AVL16_NODE(block, xpos);
        if (x->balance == +1) {
          const uint16_t wpos = x->child[1];
          z_avl16_node_t *w = Z_AVL16_NODE(block, wpos);
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
 *  PUBLIC API
 */
uint8_t *z_avl16_insert (z_avl16_head_t *head,
                         uint8_t *block,
                         uint16_t node_pos,
                         z_avl16_compare_t key_cmp,
                         const void *key,
                         void *udata)
{
  if (Z_LIKELY(head->root != 0)) {
    z_avl16_node_t *parent;
    z_avl16_node_t *node;
    uint32_t dstack;
    uint16_t *top;
    uint16_t *q;
    uint16_t pp;
    int k, dir;

    /* lookup the insertion point */
    dstack = 0;
    k = dir = 0;
    pp = head->root;
    top = q = &(head->root);
    parent = Z_AVL16_NODE(block, pp);
    while (pp != 0) {
      z_avl16_node_t *p = Z_AVL16_NODE(block, pp);
      const int cmp = key_cmp(udata, p, key);
      if (Z_UNLIKELY(cmp == 0))
        return(p->data);

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

    pp = Z_AVL16_POS(block, q);
    if (dir && pp == head->edge[1]) {
      head->edge[1] = node_pos;
    } else if (!dir && pp == head->edge[0]) {
      head->edge[0] = node_pos;
    }

    q[dir] = node_pos;
    node = Z_AVL16_NODE(block, node_pos);
    __avl16_node_init(node);
    __avl16_ibalance(block, parent, node, top, dstack);
  } else {
    z_avl16_node_t *node;
    node = Z_AVL16_NODE(block, node_pos);
    __avl16_node_init(node);
    head->root = node_pos;
    head->edge[0] = node_pos;
    head->edge[1] = node_pos;
  }
  return(NULL);
}

void z_avl16_add_edge (z_avl16_head_t *head,
                       uint8_t *block,
                       uint16_t node_pos,
                       const int edge)
{
  if (Z_LIKELY(head->root != 0)) {
    z_avl16_node_t *parent;
    z_avl16_node_t *node;
    uint16_t *top, *q;
    uint32_t dstack;
    uint16_t pp;
    int k;

    k = 0;
    dstack = 0;
    pp = head->root;
    top = q = &(head->root);
    parent = Z_AVL16_NODE(block, pp);
    while (pp != 0) {
      z_avl16_node_t *p = Z_AVL16_NODE(block, pp);
      if (p->balance != 0) {
        k = 0;
        top = q;
        parent = p;
        dstack = 0;
      }

      q = p->child;
      pp = p->child[edge];
      dstack |= (edge << k++);
    }

    q[edge] = node_pos;
    head->edge[edge] = node_pos;
    node = Z_AVL16_NODE(block, node_pos);
    __avl16_node_init(node);
    __avl16_ibalance(block, parent, node, top, dstack);
  } else {
    z_avl16_node_t *node;
    node = Z_AVL16_NODE(block, node_pos);
    __avl16_node_init(node);
    head->root = node_pos;
    head->edge[0] = node_pos;
    head->edge[1] = node_pos;
  }
}

uint16_t z_avl16_remove (z_avl16_head_t *head,
                         uint8_t *block,
                         z_avl16_compare_t key_cmp,
                         const void *key,
                         void *udata)
{
  z_avl16_node_t *stack[20];
  z_avl16_node_t *node;
  uint8_t dstack[20];
  uint16_t node_pos;
  int istack;
  int cmp;

  Z_ASSERT(head->root != 0, "root is supposed to exist");

  cmp = 1;
  istack = 0;
  node = (z_avl16_node_t *)&(head->root);
  do {
    const int dir = cmp < 0;

    stack[istack] = node;
    dstack[istack++] = dir;
    if (node->child[dir] == 0)
      return(0);

    node_pos = node->child[dir];
    node = Z_AVL16_NODE(block, node_pos);
  } while ((cmp = key_cmp(udata, node, key)) != 0);

  __avl16_dbalance(block, node, stack, dstack, istack);

  if (node_pos == head->edge[0]) {
    // TODO
    head->edge[0] = z_avl16_lookup_min(block, head->root);
  } else if (node_pos == head->edge[1]) {
    // TODO
    head->edge[1] = z_avl16_lookup_max(block, head->root);
  }
  return(node_pos);
}

uint16_t z_avl16_remove_edge (z_avl16_head_t *head,
                              uint8_t *block,
                              const int edge)
{
  z_avl16_node_t *stack[20];
  z_avl16_node_t *node;
  uint8_t dstack[20];
  uint16_t node_pos;
  int istack;

  Z_ASSERT(head->root != 0, "root is supposed to be alread inserted");

  stack[0]  = Z_CAST(z_avl16_node_t, &(head->root));
  dstack[0] = 0;
  istack    = 1;
  node_pos = head->root;
  node = Z_AVL16_NODE(block, head->root);
  while (node->child[edge] != 0) {
    stack[istack] = node;
    dstack[istack++] = edge;
    node_pos = node->child[edge];
    node = Z_AVL16_NODE(block, node_pos);
  }

  __avl16_dbalance(block, node, stack, dstack, istack);
  head->edge[edge] = z_avl16_lookup_edge(block, head->root, edge);
  return(node_pos);
}

uint16_t z_avl16_lookup (uint8_t *block,
                         uint16_t root,
                         z_avl16_compare_t key_cmp,
                         const void *key,
                         void *udata)
{
  while (root != 0) {
    z_avl16_node_t *node = Z_AVL16_NODE(block, root);
    const int cmp = key_cmp(udata, node, key);
    if (cmp > 0) {
      root = node->child[0];
    } else if (cmp < 0) {
      root = node->child[1];
    } else {
      return(root);
    }
  }
  return(0);
}

uint16_t z_avl16_lookup_edge (uint8_t *block, uint16_t root, int edge) {
  z_avl16_node_t *node = Z_AVL16_NODE(block, root);
  while (node->child[edge] != 0) {
    root = (node->child[edge]);
    node = Z_AVL16_NODE(block, root);
  }
  return(root);
}

/* ============================================================================
 *  PRIVATE Tree Iterator
 */
static void *__avl16_iter_lookup_near (z_avl16_iter_t *self,
                                       z_avl16_compare_t key_cmp,
                                       const void *key,
                                       void *udata,
                                       const int ceil_entry)
{
  const int nceil_entry = !ceil_entry;
  uint16_t node_pos;

  self->height = 0;
  self->current = 0;
  node_pos = self->root;

  self->found = 0;
  while (node_pos != 0) {
    z_avl16_node_t *node = Z_AVL16_NODE(self->block, node_pos);
    int cmp = key_cmp(udata, node, key);
    if (cmp == 0) {
      self->current = node_pos;
      self->found = 1;
      return(node->data);
    }

    if (ceil_entry ? (cmp > 0) : (cmp < 0)) {
      if (node->child[nceil_entry] != 0) {
        self->stack[(self->height)++] = node_pos;
        node_pos = node->child[nceil_entry];
      } else {
        self->current = node_pos;
        return(node->data);
      }
    } else {
      if (node->child[ceil_entry] != 0) {
        self->stack[(self->height)++] = node_pos;
        node_pos = node->child[ceil_entry];
      } else {
        if (self->height > 0) {
          z_avl16_node_t *parent;
          uint16_t parent_pos;

          parent_pos = self->stack[--(self->height)];
          parent = Z_AVL16_NODE(self->block, parent_pos);
          while (node_pos == parent->child[ceil_entry]) {
            if (self->height == 0) {
              self->current = 0;
              return(NULL);
            }

            node_pos = parent_pos;
            parent_pos = self->stack[--(self->height)];
            parent = Z_AVL16_NODE(self->block, parent_pos);
          }
          self->current = parent_pos;
          return(parent->data);
        }

        self->current = 0;
        return(NULL);
      }
    }
  }

  return(NULL);
}

static void *__avl16_iter_trav (z_avl16_iter_t *self, const int child) {
  z_avl16_node_t *node;
  uint16_t node_pos;

  if (self->current == 0)
    return(NULL);

  node_pos = self->current;
  node = Z_AVL16_NODE(self->block, node_pos);
  if (node->child[child] != 0) {
    const int nchild = !child;

    self->stack[(self->height)++] = node_pos;
    node_pos = node->child[child];
    node = Z_AVL16_NODE(self->block, node_pos);
    while (node->child[nchild] != 0) {
      self->stack[(self->height)++] = node_pos;
      node_pos = node->child[nchild];
      node = Z_AVL16_NODE(self->block, node_pos);
    }
  } else {
    uint16_t pos;
    do {
      if (self->height == 0) {
        self->current = 0;
        return(NULL);
      }

      pos = node_pos;
      node_pos = self->stack[--(self->height)];
      node = Z_AVL16_NODE(self->block, node_pos);
    } while (node->child[child] == pos);
  }

  self->current = node_pos;
  return(node->data);
}

static void *__avl16_iter_edge (z_avl16_iter_t *self, const int child) {
  self->height = 0;
  if (Z_LIKELY(self->root != 0)) {
    z_avl16_node_t *node;
    uint16_t node_pos;

    node_pos = self->root;
    node = Z_AVL16_NODE(self->block, node_pos);
    while (node->child[child] != 0) {
      self->stack[(self->height)++] = node_pos;
      node_pos = node->child[child];
      node = Z_AVL16_NODE(self->block, node_pos);
    }
    self->current = node_pos;
    return(node->data);
  }

  self->current = 0;
  return(NULL);
}

/* ============================================================================
 *  PUBLIC Tree Iterator
 */
void z_avl16_iter_init (z_avl16_iter_t *self, uint8_t *block, uint16_t root) {
  self->block = block;
  self->current = 0;
  self->height = 0;
  self->root = root;
}

void *z_avl16_iter_seek_begin (z_avl16_iter_t *self) {
  return __avl16_iter_edge(self, 0);
}

void *z_avl16_iter_seek_end (z_avl16_iter_t *self) {
  return __avl16_iter_edge(self, 1);
}

void *z_avl16_iter_seek_le (z_avl16_iter_t *iter,
                             z_avl16_compare_t key_cmp,
                             const void *key,
                             void *udata)
{
  return __avl16_iter_lookup_near(iter, key_cmp, key, udata, 0);
}

void *z_avl16_iter_seek_ge (z_avl16_iter_t *iter,
                            z_avl16_compare_t key_cmp,
                            const void *key,
                            void *udata)
{
  return(__avl16_iter_lookup_near(iter, key_cmp, key, udata, 1));
}

void *z_avl16_iter_next (z_avl16_iter_t *self) {
  return __avl16_iter_trav(self, 1);
}

void *z_avl16_iter_prev (z_avl16_iter_t *self) {
  return __avl16_iter_trav(self, 0);
}
