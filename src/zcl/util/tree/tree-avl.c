/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <zcl/debug.h>
#include <zcl/tree.h>

#if 0
  #define __DSTACK_SIZE                      z_bitmap_size(Z_TREE_MAX_HEIGHT)
  #define __dstack_change(data, idx, value)  z_bitmap_change((data)->dstack, idx, value)
  #define __dstack_set(data, idx)            z_bitmap_set((data)->dstack, idx)
  #define __dstack_clear(data, idx)          z_bitmap_clear((data)->dstack, idx)
  #define __dstack_get(data, idx)            !!z_bitmap_test((data)->dstack, idx)
#else
  #define __DSTACK_SIZE                      Z_TREE_MAX_HEIGHT
  #define __dstack_change(data, idx, value)  (data)->dstack[idx] = value
  #define __dstack_set(data, idx)            (data)->dstack[idx] = 1
  #define __dstack_clear(data, idx)          (data)->dstack[idx] = 0
  #define __dstack_get(data, idx)            (data)->dstack[idx]
#endif

struct avl_insert_data {
  uint8_t dstack[__DSTACK_SIZE]; /* Cached comparison results */
  z_tree_node_t *parent;
  z_tree_node_t *top;
};

struct avl_remove_data {
  z_tree_node_t *stack[Z_TREE_MAX_HEIGHT]; /* Nodes */
  uint8_t dstack[__DSTACK_SIZE];           /* Directions moved from stack nodes */
  int istack;                              /* Stack pointer */
};

static z_tree_node_t **__do_insert_lookup (const z_tree_info_t *tree,
                                           struct avl_insert_data *v,
                                           z_tree_node_t **root,
                                           z_tree_node_t *node,
                                           void *udata)
{
  z_tree_node_t *p, *q;
  int k, dir;

  v->top = (z_tree_node_t *)root;
  v->parent = *root;
  dir = 0;
  k = 0;
  for (q = v->top, p = v->parent; p != NULL; q = p, p = p->child[dir]) {
    int cmp = tree->node_compare(udata, p, node);
    if (Z_UNLIKELY(cmp == 0)) {
      if (Z_LIKELY(p != node)) {
        Z_ASSERT(q->child[dir] == p, "wrong parent node");
        node->child[0] = p->child[0];
        node->child[1] = p->child[1];
        node->balance  = p->balance;
        q->child[dir] = node;

        if (tree->node_free != NULL) {
          tree->node_free(udata, p);
        }
      }
      return(NULL);
    }

    if (p->balance != 0) {
      v->top = q;
      v->parent = p;
      k = 0;
    }

    dir = cmp < 0;
    __dstack_change(v, k, dir);
    ++k;
    Z_ASSERT(k < Z_TREE_MAX_HEIGHT, "AVL Insert-Stack to high %d", k);
  }

  return(&(q->child[dir]));
}

static void __do_insert (const z_tree_info_t *tree,
                         const struct avl_insert_data *v,
                         z_tree_node_t *node)
{
  z_tree_node_t *w;
  z_tree_node_t *p;
  int k;

  node->child[0] = node->child[1] = NULL;
  node->balance = 0;
  if (v->parent == NULL)
    return;

  for (p = v->parent, k = 0; p != node; p = p->child[__dstack_get(v, k)], ++k) {
    if (__dstack_get(v, k) == 0)
      --(p->balance);
    else
      ++(p->balance);
  }

  if (v->parent->balance == -2) {
    z_tree_node_t *x = v->parent->child[0];
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
      if (w->balance == -1)
        x->balance = 0, v->parent->balance = +1;
      else if (w->balance == 0)
        x->balance = v->parent->balance = 0;
      else /* |w->balance == +1| */
        x->balance = -1, v->parent->balance = 0;
      w->balance = 0;
    }
  } else if (v->parent->balance == +2) {
    z_tree_node_t *x = v->parent->child[1];
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
      if (w->balance == +1)
        x->balance = 0, v->parent->balance = -1;
      else if (w->balance == 0)
        x->balance = v->parent->balance = 0;
      else /* |w->balance == -1| */
        x->balance = +1, v->parent->balance = 0;
      w->balance = 0;
    }
  } else {
    return;
  }

  v->top->child[v->parent != v->top->child[0]] = w;
}


static void __do_remove (const z_tree_info_t *tree,
                         struct avl_remove_data *v,
                         z_tree_node_t *p)
{
  int k = v->istack;

  if (p->child[1] == NULL) {
    v->stack[k - 1]->child[__dstack_get(v, k - 1)] = p->child[0];
  } else {
    z_tree_node_t *r = p->child[1];
    if (r->child[0] == NULL) {
      r->child[0] = p->child[0];
      r->balance = p->balance;
      v->stack[k - 1]->child[__dstack_get(v, k - 1)] = r;
      __dstack_set(v, k);
      v->stack[k++] = r;
    } else {
      z_tree_node_t *s;
      int j = k++;

      for (;;) {
        __dstack_clear(v, k);
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

      v->stack[j - 1]->child[__dstack_get(v, j - 1)] = s;
      __dstack_set(v, j);
      v->stack[j] = s;
    }
  }

  while (--k > 0) {
    z_tree_node_t *y = v->stack[k];

    if (__dstack_get(v, k) == 0) {
      ++(y->balance);
      if (y->balance == +1)
        break;
      if (y->balance == +2) {
        z_tree_node_t *x = y->child[1];
        if (x->balance == -1) {
          z_tree_node_t *w;
          w = x->child[0];
          x->child[0] = w->child[1];
          w->child[1] = x;
          y->child[1] = w->child[0];
          w->child[0] = y;
          if (w->balance == +1)
            x->balance = 0, y->balance = -1;
          else if (w->balance == 0)
            x->balance = y->balance = 0;
          else /* |w->balance == -1| */
            x->balance = +1, y->balance = 0;
          w->balance = 0;
          v->stack[k - 1]->child[__dstack_get(v, k - 1)] = w;
        } else {
          y->child[1] = x->child[0];
          x->child[0] = y;
          v->stack[k - 1]->child[__dstack_get(v, k - 1)] = x;
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
        z_tree_node_t *x = y->child[0];
        if (x->balance == +1) {
          z_tree_node_t *w;
          w = x->child[1];
          x->child[1] = w->child[0];
          w->child[0] = x;
          y->child[0] = w->child[1];
          w->child[1] = y;
          if (w->balance == -1)
            x->balance = 0, y->balance = +1;
          else if (w->balance == 0)
            x->balance = y->balance = 0;
          else /* |w->balance == +1| */
            x->balance = -1, y->balance = 0;
          w->balance = 0;
          v->stack[k - 1]->child[__dstack_get(v, k - 1)] = w;
        } else {
          y->child[0] = x->child[1];
          x->child[1] = y;
          v->stack[k - 1]->child[__dstack_get(v, k - 1)] = x;
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

static int __avl_attach (const z_tree_info_t *tree,
                         z_tree_node_t **root,
                         z_tree_node_t *node,
                         void *udata)
{
  struct avl_insert_data v;
  z_tree_node_t **p;

  p = __do_insert_lookup(tree, &v, root, node, udata);
  if (Z_UNLIKELY(p == NULL))
    return(1);

  *p = node;
  __do_insert(tree, &v, node);
  return(0);
}

static z_tree_node_t *__avl_detach (const z_tree_info_t *tree,
                                    z_tree_node_t **root,
                                    const void *key,
                                    void *udata)
{
  struct avl_remove_data v;
  z_tree_node_t *p;
  int cmp;

  cmp = 1;
  v.istack = 0;
  p = (z_tree_node_t *)root;
  do {
    int dir = cmp < 0;

    v.stack[v.istack] = p;
    __dstack_change(&v, v.istack, dir);
    ++(v.istack);

    if ((p = p->child[dir]) == NULL)
      return(NULL);
  } while ((cmp = tree->key_compare(udata, p, key)) != 0);

  __do_remove(tree, &v, p);
  p->child[0] = NULL;
  p->child[1] = NULL;
  return(p);
}

static z_tree_node_t *__avl_detach_edge (const z_tree_info_t *tree,
                                         z_tree_node_t **root,
                                         int edge)
{
  struct avl_remove_data v;
  z_tree_node_t *next;
  z_tree_node_t *p;

  v.istack = 0;
  p = (z_tree_node_t *)root;
  while ((next = p->child[edge]) != NULL) {
    v.stack[v.istack] = p;
    __dstack_change(&v, v.istack, edge);
    ++(v.istack);
    p = next;
  }

  __do_remove(tree, &v, p);
  p->child[0] = NULL;
  p->child[1] = NULL;
  return(p);
}

const z_tree_plug_t z_tree_avl = {
  .attach      = __avl_attach,
  .detach      = __avl_detach,
  .detach_edge = __avl_detach_edge,
};
