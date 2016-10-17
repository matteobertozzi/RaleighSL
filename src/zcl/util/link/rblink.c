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

#include <zcl/rblink.h>

/* ============================================================================
 *  PRIVATE Red-Black Tree methods
 */
static void __tree_rb_insert_color (z_rblink_t **root, z_rblink_t *node) {
  z_rblink_t *parent, *gparent, *tmp;
  z_rblink_t *memo;
  while ((parent = node->parent) && parent->color == 1) {
    gparent = parent->parent;
    if (parent == gparent->child[0]) {
      tmp = gparent->child[1];
      if (tmp && tmp->color == 1) {
        tmp->color = 0;
        parent->color = 0;
        gparent->color = 1;
        node = gparent;
        continue;
      }

      tmp = parent->child[1];
      if (tmp == node) {
        memo = tmp->child[0];
        if ((parent->child[1] = memo)) {
          memo->parent = parent;
        }
        if ((tmp->parent = gparent)) {
          int ichild = (parent != gparent->child[0]);
          gparent->child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->child[0] = parent;
        parent->parent = tmp;

        tmp = parent;
        parent = node;
        node = tmp;
      }

      parent->color = 0;
      gparent->color = 1;

      tmp = gparent->child[0];
      memo = tmp->child[1];
      if ((gparent->child[0] = memo)) {
        memo->parent = gparent;
      }

      memo = gparent->parent;
      if ((tmp->parent = memo)) {
        int ichild = (gparent != memo->child[0]);
        memo->child[ichild] = tmp;
      } else {
        *root = tmp;
      }
      tmp->child[1] = (gparent);
      gparent->parent = tmp;
    } else {
      tmp = gparent->child[0];
      if (tmp && tmp->color == 1) {
        tmp->color = 0;
        parent->color = 0;
        gparent->color = 1;
        node = gparent;
        continue;
      }

      tmp = parent->child[0];
      if (tmp == node) {
        memo = tmp->child[1];
        if ((parent->child[0] = memo)) {
          memo->parent = parent;
        }

        memo = parent->parent;
        if ((tmp->parent = memo)) {
          int ichild = (parent != memo->child[0]);
          memo->child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->child[1] = parent;
        parent->parent = tmp;

        tmp = parent;
        parent = node;
        node = tmp;
      }

      parent->color = 0;
      gparent->color = 1;

      tmp = gparent->child[1];
      memo = tmp->child[0];
      if ((gparent->child[1] = memo)) {
        memo->parent = gparent;
      }

      memo = gparent->parent;
      if ((tmp->parent = memo)) {
        int ichild = (gparent != memo->child[0]);
        memo->child[ichild] = tmp;
      } else {
        *root = tmp;
      }
      tmp->child[0] = gparent;
      gparent->parent = tmp;
    }
  }
  (*root)->color = 0;
}

static void __tree_rb_remove_color (z_rblink_t **root, z_rblink_t *parent, z_rblink_t *node) {
  z_rblink_t *memo;
  z_rblink_t *tmp;
  while ((node == NULL || node->color == 0) && node != *root) {
    if (parent->child[0] == node) {
      tmp = parent->child[1];
      if (tmp->color == 1) {
        tmp->color = 0;
        parent->color = 1;

        tmp = parent->child[1];
        memo = tmp->child[0];
        if ((parent->child[1] = memo)) {
          memo->parent = parent;
        }

        memo = parent->parent;
        if ((tmp->parent = memo)) {
          int ichild = (parent != memo->child[0]);
          memo->child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->child[0] = parent;
        parent->parent = tmp;
        tmp = parent->child[1];
      }

      if ((tmp->child[0] == NULL || tmp->child[0]->color == 0) &&
          (tmp->child[1] == NULL || tmp->child[1]->color == 0))
      {
        tmp->color = 1;
        node = parent;
        parent = node->parent;
      } else {
        if (tmp->child[1] == NULL || tmp->child[1]->color == 0) {
          z_rblink_t *oleft;
          if ((oleft = tmp->child[0]))
            oleft->color = 0;
          tmp->color = 1;

          oleft = tmp->child[0];
          memo = oleft->child[1];
          if ((tmp->child[0] = memo)) {
            memo->parent = tmp;
          }

          memo = tmp->parent;
          if ((oleft->parent = memo)) {
            int ichild = (tmp != memo->child[0]);
            memo->child[ichild] = oleft;
          } else {
            *root = (oleft);
          }
          oleft->child[1] = tmp;
          tmp->parent = (oleft);
          tmp = parent->child[1];
        }

        tmp->color = parent->color;
        parent->color = 0;
        if ((memo = tmp->child[1]))
          memo->color = 0;

        tmp = parent->child[1];
        memo = tmp->child[0];
        if ((parent->child[1] = memo)) {
          memo->parent = parent;
        }

        memo = parent->parent;
        if ((tmp->parent = memo)) {
          int ichild = (parent != memo->child[0]);
          memo->child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->child[0] = parent;
        parent->parent = tmp;
        node = *root;
        break;
      }
    } else {
      tmp = parent->child[0];
      if (tmp->color == 1) {
        tmp->color = 0;
        parent->color = 1;

        tmp = parent->child[0];
        memo = tmp->child[1];
        if ((parent->child[0] = memo)) {
          memo->parent = parent;
        }

        memo = parent->parent;
        if ((tmp->parent = memo)) {
          int ichild = (parent != memo->child[0]);
          memo->child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->child[1] = parent;
        parent->parent = tmp;
        tmp = parent->child[0];
      } if ((tmp->child[0] == NULL || tmp->child[0]->color == 0) &&
            (tmp->child[1] == NULL || tmp->child[1]->color == 0))
      {
        tmp->color = 1;
        node = parent;
        parent = node->parent;
      } else {
        if (tmp->child[0] == NULL || tmp->child[0]->color == 0) {
          z_rblink_t *oright;
          if ((oright = tmp->child[1]))
            oright->color = 0;
          tmp->color = 1;

          oright = tmp->child[1];
          memo = oright->child[0];
          if ((tmp->child[1] = memo)) {
            memo->parent = tmp;
          }

          memo = tmp->parent;
          if ((oright->parent = memo)) {
            int ichild = (tmp == memo->child[0]);
            memo->child[ichild] = oright;
          } else {
            *root = (oright);
          }
          oright->child[0] = tmp;
          tmp->parent = (oright);
          tmp = parent->child[0];
        }

        tmp->color = parent->color;
        parent->color = 0;
        if ((memo = tmp->child[0]))
          memo->color = 0;

        tmp = parent->child[0];
        memo = tmp->child[1];
        if ((parent->child[0] = memo)) {
          memo->parent = parent;
        }

        memo = parent->parent;
        if ((tmp->parent = memo)) {
          int ichild = (parent != memo->child[0]);
          memo->child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->child[1] = parent;
        parent->parent = tmp;
        node = *root;
        break;
      }
    }
  }
  if (node) {
    node->color = 0;
  }
}

static z_rblink_t *__node_tree_remove(z_rblink_t **root, z_rblink_t *node) {
  z_rblink_t *child, *parent, *old = node;
  int color;
  if (node->child[0] == NULL) {
    child = node->child[1];
  } else if (node->child[1] == NULL) {
    child = node->child[0];
  } else {
    z_rblink_t *left;
    z_rblink_t *memo;
    node = node->child[1];
    while ((left = node->child[0]))
      node = left;
    child = node->child[1];
    parent = node->parent;
    color = node->color;
    if (child)
      child->parent = parent;
    if (parent) {
      int ichild = (parent->child[0] != node);
      parent->child[ichild] = child;
    } else {
      *root = child;
    }
    if (node->parent == old)
      parent = node;
    node->child[0] = old->child[0];
    node->child[1] = old->child[1];
    node->parent = old->parent;
    if ((memo = old->parent)) {
      int ichild = (memo->child[0] != old);
      memo->child[ichild] = node;
    } else {
      *root = node;
    }
    old->child[0]->parent = node;
    if ((memo = old->child[1]))
      memo->parent = node;
    goto color;
  }
  parent = node->parent;
  color = node->color;
  if (child)
    child->parent = parent;
  if (parent) {
    int ichild = (parent->child[0] != node);
    parent->child[ichild] = child;
  } else {
    *root = child;
  }
color:
  if (color == 0)
    __tree_rb_remove_color(root, parent, child);

  old->child[0] = NULL;
  old->child[1] = NULL;
  old->parent = NULL;
  return(old);
}

/* ============================================================================
 *  PUBLIC Tree methods
 */
void z_rb_tree_init (z_rbtree_t *self) {
  self->root = NULL;
}

z_rblink_t *z_rb_tree_insert (z_rbtree_t *self,
                              z_rb_node_cmp_t cmp_func,
                              z_rblink_t *node,
                              void *udata)
{
  z_rblink_t *parent = NULL;
  z_rblink_t *tmp;
  int dir = 0;

  tmp = self->root;
  while (tmp) {
    const int cmp = cmp_func(udata, node, tmp);
    if (Z_UNLIKELY(cmp == 0)) return tmp;

    parent = tmp;
    dir = (cmp >= 0);
    tmp = tmp->child[dir];
  }

  node->parent = parent;
  node->child[0] = node->child[1] = NULL;
  node->color = 1;
  if (parent != NULL) {
    parent->child[dir] = node;
  } else {
    self->root = node;
  }
  __tree_rb_insert_color(&(self->root), node);
  return(NULL);
}

int z_rb_tree_remove (z_rbtree_t *self,
                      z_rb_key_cmp_t cmp_func,
                      const void *key,
                      void *udata)
{
  z_rblink_t *node;

  node = z_rb_tree_lookup(self, cmp_func, key, udata);
  if (node == NULL) return(0);

  __node_tree_remove(&(self->root), node);
  return(1);
}

z_rblink_t * z_rb_tree_lookup (const z_rbtree_t *self,
                               z_rb_key_cmp_t key_cmp,
                               const void *key,
                               void *udata)
{
  z_rblink_t *node = self->root;
  while (node != NULL) {
    const int cmp = key_cmp(udata, key, node);
    if (cmp == 0) return(node);
    const int dir = (cmp > 0);
    node = node->child[dir];
  }
  return(NULL);
}

z_rblink_t *z_rb_tree_lookup_edge (const z_rbtree_t *self, const int edge) {
  z_rblink_t *node = self->root;
  while (node->child[edge] != NULL) {
    node = node->child[edge];
  }
  return(node);
}

/* ============================================================================
 *  PUBLIC Link Iterator
 */
z_rblink_t *z_rb_link_next (z_rblink_t *node) {
  if (node->child[1]) {
    node = node->child[1];
    while (node->child[0])
      node = node->child[0];
  } else {
    if (node->parent && (node == (node->parent)->child[0])) {
      node = node->parent;
    } else {
      while (node->parent && (node == (node->parent)->child[1]))
        node = node->parent;
      node = node->parent;
    }
  }
  return node;
}

z_rblink_t *z_rb_link_prev (z_rblink_t *node) {
  if (node->child[0]) {
    node = node->child[0];
    while (node->child[1])
      node = node->child[1];
  } else {
    if (node->parent && (node == (node->parent)->child[1])) {
      node = node->parent;
    } else {
      while (node->parent && (node == (node->parent)->child[0]))
        node = node->parent;
      node = node->parent;
    }
  }
  return node;
}

int z_rb_link_has_next (z_rblink_t *node) {
  return z_rb_link_next(node) != NULL;
}

int z_rb_link_has_prev (z_rblink_t *node) {
  return z_rb_link_prev(node) != NULL;
}

/* ============================================================================
 *  PRIVATE Tree Iterator
 */
static z_rblink_t *__rb_iter_lookup_near (z_rb_iter_t *self,
                                          z_rb_key_cmp_t key_cmp,
                                          const void *key,
                                          void *udata,
                                          const int ceil_entry)
{
  const int nceil_entry = !ceil_entry;
  z_rblink_t *node;

  self->current = NULL;
  self->found = 0;

  node = self->tree->root;
  while (node != NULL) {
    const int cmp = key_cmp(udata, key, node);
    if (cmp == 0) {
      self->current = node;
      self->found = 1;
      return(node);
    }

    if (ceil_entry ? (cmp < 0) : (cmp > 0)) {
      if (node->child[nceil_entry] != NULL) {
        node = node->child[nceil_entry];
      } else {
        self->current = node;
        return(node);
      }
    } else {
      if (node->child[ceil_entry] != NULL) {
        node = node->child[ceil_entry];
      } else {
        if (node->parent != NULL) {
          z_rblink_t *parent;

          parent = node->parent;
          while (node == parent->child[ceil_entry]) {
            if (parent->parent == NULL) {
              self->current = NULL;
              return(NULL);
            }

            node = parent;
            parent = node->parent;
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

/* ============================================================================
 *  PUBLIC Tree Iterator
 */
void z_rb_iter_init (z_rb_iter_t *self, z_rbtree_t *tree) {
  self->current = NULL;
  self->tree = tree;
}

z_rblink_t *z_rb_iter_seek_begin (z_rb_iter_t *self) {
  self->current = z_rb_tree_lookup_edge(self->tree, 0);
  return self->current;
}

z_rblink_t *z_rb_iter_seek_end (z_rb_iter_t *self) {
  self->current = z_rb_tree_lookup_edge(self->tree, 1);
  return self->current;
}

z_rblink_t *z_rb_iter_seek_le (z_rb_iter_t *iter,
                               z_rb_key_cmp_t key_cmp,
                               const void *key,
                               void *udata)
{
  return __rb_iter_lookup_near(iter, key_cmp, key, udata, 0);
}

z_rblink_t *z_rb_iter_seek_ge (z_rb_iter_t *iter,
                               z_rb_key_cmp_t key_cmp,
                               const void *key,
                               void *udata)
{
  return __rb_iter_lookup_near(iter, key_cmp, key, udata, 1);
}

z_rblink_t *z_rb_iter_next (z_rb_iter_t *self) {
  self->current = z_rb_link_next(self->current);
  return self->current;
}

z_rblink_t *z_rb_iter_prev (z_rb_iter_t *self) {
  self->current = z_rb_link_prev(self->current);
  return self->current;
}
