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

#include <zcl/avl-link.h>

typedef struct avl_insert_data {
  z_avl_link_t *parent;
  z_avl_link_t *top;
  int dstack;
} z_avl_insert_data_t;

typedef struct z_avl_remove_data {
  z_avl_link_t *stack[32];    /* Nodes */
  uint8_t dstack[32];         /* Directions moved from stack nodes */
  int istack;                 /* Stack pointer */
} z_avl_remove_data_t;

/* ============================================================================
 *  PRIVATE methods
 */
static void __avl_insert_balance (const z_avl_insert_data_t *v,
                                  z_avl_link_t *node)
{
  z_avl_link_t *w;

  do {
    z_avl_link_t *p = v->parent;
    uint64_t dstack = v->dstack;
    while (p != node) {
      const int dir = dstack & 1;
      p->balance += (dir ? 1 : -1);
      p = p->child[dir];
      dstack >>= 1;
    }
  } while (0);

  if (v->parent->balance == -2) {
    z_avl_link_t *x = v->parent->child[0];
    if (x->balance == -1) {
      w = x;
      v->parent->child[0] = x->child[1];
      x->child[1] = v->parent;
      x->balance = v->parent->balance = 0;
    } else {
      w = x->child[1];
      x->child[1] = w->child[0];
      w->child[0] = x;
      v->parent->child[0] = w->child[1];
      w->child[1] = v->parent;
      if (w->balance == -1) {
        x->balance = 0, v->parent->balance = +1;
      } else if (w->balance == 0) {
        x->balance = v->parent->balance = 0;
      } else /* |w->balance == +1| */ {
        x->balance = -1, v->parent->balance = 0;
      }
      w->balance = 0;
    }
  } else if (v->parent->balance == +2) {
    z_avl_link_t *x = v->parent->child[1];
    if (x->balance == +1) {
      w = x;
      v->parent->child[1] = x->child[0];
      x->child[0] = v->parent;
      x->balance = v->parent->balance = 0;
    } else {
      w = x->child[0];
      x->child[0] = w->child[1];
      w->child[1] = x;
      v->parent->child[1] = w->child[0];
      w->child[0] = v->parent;
      if (w->balance == +1) {
        x->balance = 0, v->parent->balance = -1;
      } else if (w->balance == 0) {
        x->balance = v->parent->balance = 0;
      } else /* |w->balance == -1| */ {
        x->balance = +1, v->parent->balance = 0;
      }
      w->balance = 0;
    }
  } else {
    return;
  }

  v->top->child[v->parent != v->top->child[0]] = w;
}

static void __avl_remove_balance (z_avl_remove_data_t *v, z_avl_link_t *p) {
  int k = v->istack;

  if (p->child[1] == NULL) {
    v->stack[k - 1]->child[v->dstack[k - 1]] = p->child[0];
  } else {
    z_avl_link_t *r = p->child[1];
    if (r->child[0] == NULL) {
      r->child[0] = p->child[0];
      r->balance = p->balance;
      v->stack[k - 1]->child[v->dstack[k - 1]] = r;
      v->dstack[k] = 1;
      v->stack[k++] = r;
    } else {
      z_avl_link_t *s;
      int j = k++;

      for (;;) {
        v->dstack[k] = 0;
        v->stack[k++] = r;
        s = r->child[0];
        if (s->child[0] == NULL)
          break;

        r = s;
      }

      s->child[0] = p->child[0];
      r->child[0] = s->child[1];
      s->child[1] = p->child[1];
      s->balance = p->balance;

      v->stack[j - 1]->child[v->dstack[j - 1]] = s;
      v->dstack[j] = 1;
      v->stack[j] = s;
    }
  }

  while (--k > 0) {
    z_avl_link_t *y = v->stack[k];

    if (v->dstack[k] == 0) {
      ++(y->balance);
      if (y->balance == +1)
        break;
      if (y->balance == +2) {
        z_avl_link_t *x = y->child[1];
        if (x->balance == -1) {
          z_avl_link_t *w = x->child[0];
          x->child[0] = w->child[1];
          w->child[1] = x;
          y->child[1] = w->child[0];
          w->child[0] = y;
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
          v->stack[k - 1]->child[v->dstack[k - 1]] = w;
        } else {
          y->child[1] = x->child[0];
          x->child[0] = y;
          v->stack[k - 1]->child[v->dstack[k - 1]] = x;
          if (x->balance == 0) {
            x->balance = -1;
            y->balance = +1;
            break;
          } else {
            x->balance = y->balance = 0;
          }
        }
      }
    } else {
      --(y->balance);
      if (y->balance == -1)
        break;
      if (y->balance == -2) {
        z_avl_link_t *x = y->child[0];
        if (x->balance == +1) {
          z_avl_link_t *w = x->child[1];
          x->child[1] = w->child[0];
          w->child[0] = x;
          y->child[0] = w->child[1];
          w->child[1] = y;
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
          v->stack[k - 1]->child[v->dstack[k - 1]] = w;
        } else {
          y->child[0] = x->child[1];
          x->child[1] = y;
          v->stack[k - 1]->child[v->dstack[k - 1]] = x;
          if (x->balance == 0) {
            x->balance = +1;
            y->balance = -1;
            break;
          } else {
            x->balance = y->balance = 0;
          }
        }
      }
    }
  }
}

/* ============================================================================
 *  PUBLIC methods
 */
void z_avl_tree_init (z_avl_tree_t *self) {
  self->root = NULL;
}

z_avl_link_t *z_avl_tree_insert (z_avl_tree_t *self,
                                 z_avl_node_cmp_t cmp_func,
                                 z_avl_link_t *node,
                                 void *udata)
{
  node->child[0] = NULL;
  node->child[1] = NULL;
  node->balance = 0;

  if (self->root != NULL) {
    z_avl_insert_data_t v;
    z_avl_link_t *p, *q;
    int k = 0;
    int dir;

    /* lookup the insertion point */
    k = dir = 0;
    v.top = q = Z_CAST(z_avl_link_t, &(self->root));
    v.parent = p = self->root;
    v.dstack = 0;
    while (p != NULL) {
      const int cmp = cmp_func(udata, p, node);
      if (Z_UNLIKELY(cmp == 0)) {
        return(p);
      }

      if (p->balance != 0) {
        k = 0;
        v.top = q;
        v.parent = p;
        v.dstack = 0;
      }

      dir = cmp < 0;
      q = p;
      p = p->child[dir];
      v.dstack |= (dir << k++);
    }

    q->child[dir] = node;
    __avl_insert_balance(&v, node);
  } else {
    self->root = node;
  }
  return(NULL);
}

int z_avl_tree_remove (z_avl_tree_t *self,
                       z_avl_key_cmp_t cmp_func,
                       const void *key,
                       void *udata)
{
  z_avl_remove_data_t v;
  z_avl_link_t *p;
  int cmp = -1;

  if (self->root == NULL)
    return(0);

  v.istack = 0;
  p = Z_CAST(z_avl_link_t, &(self->root));
  do {
    const int dir = cmp >= 0;

    v.stack[v.istack] = p;
    v.dstack[v.istack++] = dir;
    if (p->child[dir] == 0)
      return(0);

    p = p->child[dir];
  } while ((cmp = cmp_func(udata, key, p)) != 0);

  __avl_remove_balance(&v, p);
  p->child[0] = NULL;
  p->child[1] = NULL;
  return(1);
}

int z_avl_tree_remove_edge (z_avl_tree_t *self,
                            z_avl_link_t **node,
                            const int edge)
{
  z_avl_remove_data_t v;
  z_avl_link_t *next;
  z_avl_link_t *p;

  if (self->root == NULL)
    return(0);

  v.istack = 0;
  p = Z_CAST(z_avl_link_t, &(self->root));
  v.stack[v.istack] = p;
  v.dstack[v.istack++] = 0;
  p = p->child[0];

  while ((next = p->child[edge]) != NULL) {
    v.stack[v.istack] = p;
    v.dstack[v.istack++] = edge;
    p = next;
  }

  *node = p;
  __avl_remove_balance(&v, p);
  p->child[0] = NULL;
  p->child[1] = NULL;
  return(1);
}

z_avl_link_t *z_avl_tree_lookup (const z_avl_tree_t *self,
                                 z_avl_key_cmp_t key_cmp,
                                 const void *key,
                                 void *udata)
{
  z_avl_link_t *node = self->root;
  while (node != NULL) {
    const int cmp = key_cmp(udata, key, node);
    if (cmp == 0) return(node);
    const int dir = (cmp > 0);
    node = node->child[dir];
  }
  return(NULL);
}

z_avl_link_t *z_avl_tree_lookup_edge (const z_avl_tree_t *self,
                                      const int edge)
{
  z_avl_link_t *node = self->root;
  while (node->child[edge] != NULL) {
    node = node->child[edge];
  }
  return(node);
}


/* ============================================================================
 *  PRIVATE Tree Iterator
 */
static z_avl_link_t *__avl_iter_lookup_near (z_avl_iter_t *self,
                                             z_avl_key_cmp_t key_cmp,
                                             const void *key,
                                             void *udata,
                                             const int ceil_entry)
{
  const int nceil_entry = !ceil_entry;
  z_avl_link_t *node;

  self->height = 0;
  self->current = NULL;
  node = self->root;

  self->found = 0;
  while (node != NULL) {
    const int cmp = key_cmp(udata, key, node);
    if (cmp == 0) {
      self->current = node;
      self->found = 1;
      return(node);
    }

    if (ceil_entry ? (cmp < 0) : (cmp > 0)) {
      if (node->child[nceil_entry] != NULL) {
        self->stack[(self->height)++] = node;
        node = node->child[nceil_entry];
      } else {
        self->current = node;
        return(node);
      }
    } else {
      if (node->child[ceil_entry] != NULL) {
        self->stack[(self->height)++] = node;
        node = node->child[ceil_entry];
      } else {
        if (self->height > 0) {
          z_avl_link_t *parent;

          parent = self->stack[--(self->height)];
          while (node == parent->child[ceil_entry]) {
            if (self->height == 0) {
              self->current = NULL;
              return(NULL);
            }

            node = parent;
            parent = self->stack[--(self->height)];
          }
          self->current = parent;
          return(parent);
        }

        self->current = NULL;
        return(NULL);
      }
    }
  }
  return(NULL);
}

static z_avl_link_t *__avl_iter_trav (z_avl_iter_t *self, const int child) {
  z_avl_link_t *node;

  if (self->current == 0)
    return(NULL);

  node = self->current;
  if (node->child[child] != NULL) {
    const int nchild = !child;

    self->stack[(self->height)++] = node;
    node = node->child[child];
    while (node->child[nchild] != NULL) {
      self->stack[(self->height)++] = node;
      node = node->child[nchild];
    }
  } else {
    z_avl_link_t *pos;
    do {
      if (self->height == 0) {
        self->current = 0;
        return(NULL);
      }

      pos = node;
      node = self->stack[--(self->height)];
    } while (node->child[child] == pos);
  }

  self->current = node;
  return(node);
}

static z_avl_link_t *__avl_iter_edge (z_avl_iter_t *self, const int child) {
  self->height = 0;
  if (Z_LIKELY(self->root != NULL)) {
    z_avl_link_t *node;

    node = self->root;
    while (node->child[child] != NULL) {
      self->stack[(self->height)++] = node;
      node = node->child[child];
    }
    self->current = node;
    return(node);
  }

  self->current = NULL;
  return(NULL);
}

/* ============================================================================
 *  PUBLIC Tree Iterator
 */
void z_avl_iter_init (z_avl_iter_t *self, z_avl_tree_t *tree) {
  self->current = 0;
  self->height = 0;
  self->root = tree->root;
}

z_avl_link_t *z_avl_iter_seek_begin (z_avl_iter_t *self) {
  return __avl_iter_edge(self, 0);
}

z_avl_link_t *z_avl_iter_seek_end (z_avl_iter_t *self) {
  return __avl_iter_edge(self, 1);
}

z_avl_link_t *z_avl_iter_seek_le (z_avl_iter_t *iter,
                                  z_avl_key_cmp_t key_cmp,
                                  const void *key,
                                  void *udata)
{
  return __avl_iter_lookup_near(iter, key_cmp, key, udata, 0);
}

z_avl_link_t *z_avl_iter_seek_ge (z_avl_iter_t *iter,
                                  z_avl_key_cmp_t key_cmp,
                                  const void *key,
                                  void *udata)
{
  return(__avl_iter_lookup_near(iter, key_cmp, key, udata, 1));
}

z_avl_link_t *z_avl_iter_next (z_avl_iter_t *self) {
  return __avl_iter_trav(self, 1);
}

z_avl_link_t *z_avl_iter_prev (z_avl_iter_t *self) {
  return __avl_iter_trav(self, 0);
}
