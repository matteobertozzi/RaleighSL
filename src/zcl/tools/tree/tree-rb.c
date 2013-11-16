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

#include <zcl/global.h>
#include <zcl/bitmap.h>
#include <zcl/tree.h>

#define __BLACK                            (0)
#define __RED                              (1)
#define __RB_MAX_HEIGHT                    (Z_TREE_MAX_HEIGHT * 2)

#if 0
  #define __DSTACK_SIZE                      z_bitmap_size(__RB_MAX_HEIGHT)
  #define __dstack_change(data, idx, value)  z_bitmap_change((data)->dstack, idx, value)
  #define __dstack_set(data, idx)            z_bitmap_set((data)->dstack, idx)
  #define __dstack_clear(data, idx)          z_bitmap_clear((data)->dstack, idx)
  #define __dstack_get(data, idx)            !!z_bitmap_test((data)->dstack, idx)
#else
  #define __DSTACK_SIZE                      __RB_MAX_HEIGHT
  #define __dstack_change(data, idx, value)  (data)->dstack[idx] = value
  #define __dstack_set(data, idx)            (data)->dstack[idx] = 1
  #define __dstack_clear(data, idx)          (data)->dstack[idx] = 0
  #define __dstack_get(data, idx)            (data)->dstack[idx]
#endif

struct rb_data {
  z_tree_node_t *stack[__RB_MAX_HEIGHT];   /* Nodes on stack */
  uint8_t dstack[__DSTACK_SIZE];           /* Directions moved from stack nodes */
  int istack;                              /* Stack height */
};

static z_tree_node_t **__do_insert_lookup (const z_tree_info_t *tree,
                                           struct rb_data *v,
                                           z_tree_node_t **root,
                                           void *data)
{
  z_tree_node_t *p;

  v->stack[0] = (z_tree_node_t *)root;
  __dstack_clear(v, 0);
  v->istack = 1;

  p = *root;
  while (p != NULL) {
    int cmp = tree->key_compare(tree->user_data, p->data, data);
    if (Z_UNLIKELY(cmp == 0)) {
      if (p->data != data && tree->data_free != NULL)
        tree->data_free(tree->user_data, p->data);
      p->data = data;
      return(NULL);
    }

    cmp = cmp < 0;
    v->stack[v->istack] = p;
    p = p->child[cmp];
    __dstack_change(v, v->istack, cmp);
    ++(v->istack);
    Z_ASSERT(v->istack < __RB_MAX_HEIGHT, "Red-Black Tree Stack to high %d", v->istack);
  }

  return(&(v->stack[v->istack-1]->child[__dstack_get(v, v->istack-1)]));
}

static void __do_insert (const z_tree_info_t *tree,
                         struct rb_data *v,
                         z_tree_node_t **root,
                         z_tree_node_t *node)
{
  node->child[0] = node->child[1] = NULL;
  node->balance = __RED;

  while (v->istack >= 3 && v->stack[v->istack - 1]->balance == __RED) {
    if (__dstack_get(v, v->istack - 2) == 0) {
      z_tree_node_t *y = v->stack[v->istack - 2]->child[1];
      if (y != NULL && y->balance == __RED) {
          v->stack[--(v->istack)]->balance = y->balance = __BLACK;
          v->stack[--(v->istack)]->balance = __RED;
      } else {
        z_tree_node_t *x;

        if (__dstack_get(v, v->istack - 1) == 0) {
          y = v->stack[v->istack - 1];
        } else {
          x = v->stack[v->istack - 1];
          y = x->child[1];
          x->child[1] = y->child[0];
          y->child[0] = x;
          v->stack[v->istack - 2]->child[0] = y;
        }

        x = v->stack[v->istack - 2];
        x->balance = __RED;
        y->balance = __BLACK;

        x->child[0] = y->child[1];
        y->child[1] = x;
        v->stack[v->istack - 3]->child[__dstack_get(v, v->istack - 3)] = y;
        break;
      }
    } else {
      z_tree_node_t *y = v->stack[v->istack - 2]->child[0];
      if (y != NULL && y->balance == __RED) {
          v->stack[--(v->istack)]->balance = y->balance = __BLACK;
          v->stack[--(v->istack)]->balance = __RED;
      } else {
        z_tree_node_t *x;

        if (__dstack_get(v, v->istack - 1) == 1) {
          y = v->stack[v->istack - 1];
        } else {
          x = v->stack[v->istack - 1];
          y = x->child[0];
          x->child[0] = y->child[1];
          y->child[1] = x;
          v->stack[v->istack - 2]->child[1] = y;
        }

        x = v->stack[v->istack - 2];
        x->balance = __RED;
        y->balance = __BLACK;

        x->child[1] = y->child[0];
        y->child[0] = x;
        v->stack[v->istack - 3]->child[__dstack_get(v, v->istack - 3)] = y;
        break;
      }
    }
  }
  (*root)->balance = __BLACK;
}

static void __do_remove (const z_tree_info_t *tree,
                         struct rb_data *v,
                         z_tree_node_t *p)
{
  if (p->child[1] == NULL) {
    v->stack[v->istack - 1]->child[__dstack_get(v, v->istack - 1)] = p->child[0];
  } else {
    int t;
    z_tree_node_t *r = p->child[1];
    if (r->child[0] == NULL) {
      r->child[0] = p->child[0];
      t = r->balance;
      r->balance = p->balance;
      p->balance = t;
      v->stack[v->istack - 1]->child[__dstack_get(v, v->istack - 1)] = r;
      __dstack_set(v, v->istack);
      v->stack[v->istack++] = r;
    } else {
      z_tree_node_t *s;
      int j = v->istack++;

      for (;;) {
        __dstack_clear(v, v->istack);
        v->stack[v->istack++] = r;
        s = r->child[0];
        if (s->child[0] == NULL)
          break;

        r = s;
      }

      __dstack_set(v, j);
      v->stack[j] = s;
      v->stack[j - 1]->child[__dstack_get(v, j - 1)] = s;

      s->child[0] = p->child[0];
      r->child[0] = s->child[1];
      s->child[1] = p->child[1];

      t = s->balance;
      s->balance = p->balance;
      p->balance = t;
    }
  }

  if (p->balance == __BLACK) {
    for (;;) {
      z_tree_node_t *x = v->stack[v->istack - 1]->child[__dstack_get(v, v->istack - 1)];
      if (x != NULL && x->balance == __RED) {
        x->balance = __BLACK;
        break;
      }
      if (v->istack < 2)
        break;

      if (__dstack_get(v, v->istack - 1) == 0) {
        z_tree_node_t *w = v->stack[v->istack - 1]->child[1];

        if (w->balance == __RED) {
          w->balance = __BLACK;
          v->stack[v->istack - 1]->balance = __RED;

          v->stack[v->istack - 1]->child[1] = w->child[0];
          w->child[0] = v->stack[v->istack - 1];
          v->stack[v->istack - 2]->child[__dstack_get(v, v->istack - 2)] = w;

          v->stack[v->istack] = v->stack[v->istack - 1];
          __dstack_clear(v, v->istack);
          v->stack[v->istack - 1] = w;
          v->istack++;

          w = v->stack[v->istack - 1]->child[1];
        }

        if ((w->child[0] == NULL || w->child[0]->balance == __BLACK) &&
            (w->child[1] == NULL || w->child[1]->balance == __BLACK))
        {
          w->balance = __RED;
        } else {
          if (w->child[1] == NULL || w->child[1]->balance == __BLACK) {
            z_tree_node_t *y = w->child[0];
            y->balance = __BLACK;
            w->balance = __RED;
            w->child[0] = y->child[1];
            y->child[1] = w;
            w = v->stack[v->istack - 1]->child[1] = y;
          }

          w->balance = v->stack[v->istack - 1]->balance;
          v->stack[v->istack - 1]->balance = __BLACK;
          w->child[1]->balance = __BLACK;

          v->stack[v->istack - 1]->child[1] = w->child[0];
          w->child[0] = v->stack[v->istack - 1];
          v->stack[v->istack - 2]->child[__dstack_get(v, v->istack - 2)] = w;
          break;
        }
      } else {
        z_tree_node_t *w = v->stack[v->istack - 1]->child[0];

        if (w->balance == __RED) {
          w->balance = __BLACK;
          v->stack[v->istack - 1]->balance = __RED;

          v->stack[v->istack - 1]->child[0] = w->child[1];
          w->child[1] = v->stack[v->istack - 1];
          v->stack[v->istack - 2]->child[__dstack_get(v, v->istack - 2)] = w;

          v->stack[v->istack] = v->stack[v->istack - 1];
          __dstack_set(v, v->istack);
          v->stack[v->istack - 1] = w;
          v->istack++;

          w = v->stack[v->istack - 1]->child[0];
        }

        if ((w->child[0] == NULL || w->child[0]->balance == __BLACK) &&
            (w->child[1] == NULL || w->child[1]->balance == __BLACK))
        {
          w->balance = __RED;
        } else {
          if (w->child[0] == NULL || w->child[0]->balance == __BLACK) {
            z_tree_node_t *y = w->child[1];
            y->balance = __BLACK;
            w->balance = __RED;
            w->child[1] = y->child[0];
            y->child[0] = w;
            w = v->stack[v->istack - 1]->child[0] = y;
          }

          w->balance = v->stack[v->istack - 1]->balance;
          v->stack[v->istack - 1]->balance = __BLACK;
          w->child[0]->balance = __BLACK;

          v->stack[v->istack - 1]->child[0] = w->child[1];
          w->child[1] = v->stack[v->istack - 1];
          v->stack[v->istack - 2]->child[__dstack_get(v, v->istack - 2)] = w;
          break;
        }
      }
      --(v->istack);
    }
  }
}

static int __redblack_attach (const z_tree_info_t *tree,
                              z_tree_node_t **root,
                              z_tree_node_t *node)
{
  z_tree_node_t **p;
  struct rb_data v;

  p = __do_insert_lookup(tree, &v, root, node->data);
  if (Z_UNLIKELY(p == NULL))
    return(1);

  *p = node;
  __do_insert(tree, &v, root, node);
  return(0);
}

static z_tree_node_t *__redblack_detach (const z_tree_info_t *tree,
                                         z_tree_node_t **root,
                                         const void *key)
{
  struct rb_data v;
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
  } while ((cmp = tree->key_compare(tree->user_data, p->data, key)) != 0);

  __do_remove(tree, &v, p);
  p->child[0] = NULL;
  p->child[1] = NULL;
  return(p);
}

static z_tree_node_t *__redblack_detach_edge (const z_tree_info_t *tree,
                                              z_tree_node_t **root,
                                              int edge)
{
  struct rb_data v;
  z_tree_node_t *p;

  v.istack = 0;
  p = (z_tree_node_t *)root;
  while (p->child[edge] != NULL) {
    v.stack[v.istack] = p;
    __dstack_change(&v, v.istack, edge);
    ++(v.istack);
    p = p->child[edge];
  }

  __do_remove(tree, &v, p);
  p->child[0] = NULL;
  p->child[1] = NULL;
  return(p);
}

const z_tree_plug_t z_tree_red_black = {
  .attach      = __redblack_attach,
  .detach      = __redblack_detach,
  .detach_edge = __redblack_detach_edge,
};
