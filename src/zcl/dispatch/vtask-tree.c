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
#include <zcl/vtask.h>

/* ============================================================================
 *  PRIVATE Task Red-Black Tree comparer method
 */
#define __vtask_compare(a, b)               \
  (((a)->vtime != b->vtime) ?               \
    (long)((a)->vtime - (b)->vtime) :       \
    (long)((a)->seqid - (b)->seqid))

/* ============================================================================
 *  PRIVATE Task Red-Black Tree methods
 */
static void __tree_rb_insert_color (z_vtask_t **root, z_vtask_t *vtask) {
  z_vtask_t *parent, *gparent, *tmp;
  z_vtask_t *memo;
  while ((parent = vtask->link.parent) && parent->flags.rblink == 1) {
    gparent = parent->link.parent;
    if (parent == gparent->link.child[0]) {
      tmp = gparent->link.child[1];
      if (tmp && tmp->flags.rblink == 1) {
        tmp->flags.rblink = 0;
        parent->flags.rblink = 0;
        gparent->flags.rblink = 1;
        vtask = gparent;
        continue;
      }

      tmp = parent->link.child[1];
      if (tmp == vtask) {
        memo = tmp->link.child[0];
        if ((parent->link.child[1] = memo)) {
          memo->link.parent = parent;
        }
        if ((tmp->link.parent = gparent)) {
          int ichild = (parent != gparent->link.child[0]);
          gparent->link.child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->link.child[0] = parent;
        parent->link.parent = tmp;

        tmp = parent;
        parent = vtask;
        vtask = tmp;
      }

      parent->flags.rblink = 0;
      gparent->flags.rblink = 1;

      tmp = gparent->link.child[0];
      memo = tmp->link.child[1];
      if ((gparent->link.child[0] = memo)) {
        memo->link.parent = gparent;
      }

      memo = gparent->link.parent;
      if ((tmp->link.parent = memo)) {
        int ichild = (gparent != memo->link.child[0]);
        memo->link.child[ichild] = tmp;
      } else {
        *root = tmp;
      }
      tmp->link.child[1] = (gparent);
      gparent->link.parent = tmp;
    } else {
      tmp = gparent->link.child[0];
      if (tmp && tmp->flags.rblink == 1) {
        tmp->flags.rblink = 0;
        parent->flags.rblink = 0;
        gparent->flags.rblink = 1;
        vtask = gparent;
        continue;
      }

      tmp = parent->link.child[0];
      if (tmp == vtask) {
        memo = tmp->link.child[1];
        if ((parent->link.child[0] = memo)) {
          memo->link.parent = parent;
        }

        memo = parent->link.parent;
        if ((tmp->link.parent = memo)) {
          int ichild = (parent != memo->link.child[0]);
          memo->link.child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->link.child[1] = parent;
        parent->link.parent = tmp;

        tmp = parent;
        parent = vtask;
        vtask = tmp;
      }

      parent->flags.rblink = 0;
      gparent->flags.rblink = 1;

      tmp = gparent->link.child[1];
      memo = tmp->link.child[0];
      if ((gparent->link.child[1] = memo)) {
        memo->link.parent = gparent;
      }

      memo = gparent->link.parent;
      if ((tmp->link.parent = memo)) {
        int ichild = (gparent != memo->link.child[0]);
        memo->link.child[ichild] = tmp;
      } else {
        *root = tmp;
      }
      tmp->link.child[0] = gparent;
      gparent->link.parent = tmp;
    }
  }
  (*root)->flags.rblink = 0;
}

static void __tree_rb_remove_color (z_vtask_t **root, z_vtask_t *parent, z_vtask_t *vtask) {
  z_vtask_t *memo;
  z_vtask_t *tmp;
  while ((vtask == NULL || vtask->flags.rblink == 0) && vtask != *root) {
    if (parent->link.child[0] == vtask) {
      tmp = parent->link.child[1];
      if (tmp->flags.rblink == 1) {
        tmp->flags.rblink = 0;
        parent->flags.rblink = 1;

        tmp = parent->link.child[1];
        memo = tmp->link.child[0];
        if ((parent->link.child[1] = memo)) {
          memo->link.parent = parent;
        }

        memo = parent->link.parent;
        if ((tmp->link.parent = memo)) {
          int ichild = (parent != memo->link.child[0]);
          memo->link.child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->link.child[0] = parent;
        parent->link.parent = tmp;
        tmp = parent->link.child[1];
      }

      if ((tmp->link.child[0] == NULL || tmp->link.child[0]->flags.rblink == 0) &&
          (tmp->link.child[1] == NULL || tmp->link.child[1]->flags.rblink == 0))
      {
        tmp->flags.rblink = 1;
        vtask = parent;
        parent = vtask->link.parent;
      } else {
        if (tmp->link.child[1] == NULL || tmp->link.child[1]->flags.rblink == 0) {
          z_vtask_t *oleft;
          if ((oleft = tmp->link.child[0]))
            oleft->flags.rblink = 0;
          tmp->flags.rblink = 1;

          oleft = tmp->link.child[0];
          memo = oleft->link.child[1];
          if ((tmp->link.child[0] = memo)) {
            memo->link.parent = tmp;
          }

          memo = tmp->link.parent;
          if ((oleft->link.parent = memo)) {
            int ichild = (tmp != memo->link.child[0]);
            memo->link.child[ichild] = oleft;
          } else {
            *root = (oleft);
          }
          oleft->link.child[1] = tmp;
          tmp->link.parent = (oleft);
          tmp = parent->link.child[1];
        }

        tmp->flags.rblink = parent->flags.rblink;
        parent->flags.rblink = 0;
        if ((memo = tmp->link.child[1]))
          memo->flags.rblink = 0;

        tmp = parent->link.child[1];
        memo = tmp->link.child[0];
        if ((parent->link.child[1] = memo)) {
          memo->link.parent = parent;
        }

        memo = parent->link.parent;
        if ((tmp->link.parent = memo)) {
          int ichild = (parent != memo->link.child[0]);
          memo->link.child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->link.child[0] = parent;
        parent->link.parent = tmp;
        vtask = *root;
        break;
      }
    } else {
      tmp = parent->link.child[0];
      if (tmp->flags.rblink == 1) {
        tmp->flags.rblink = 0;
        parent->flags.rblink = 1;

        tmp = parent->link.child[0];
        memo = tmp->link.child[1];
        if ((parent->link.child[0] = memo)) {
          memo->link.parent = parent;
        }

        memo = parent->link.parent;
        if ((tmp->link.parent = memo)) {
          int ichild = (parent != memo->link.child[0]);
          memo->link.child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->link.child[1] = parent;
        parent->link.parent = tmp;
        tmp = parent->link.child[0];
      } if ((tmp->link.child[0] == NULL || tmp->link.child[0]->flags.rblink == 0) &&
            (tmp->link.child[1] == NULL || tmp->link.child[1]->flags.rblink == 0))
      {
        tmp->flags.rblink = 1;
        vtask = parent;
        parent = vtask->link.parent;
      } else {
        if (tmp->link.child[0] == NULL || tmp->link.child[0]->flags.rblink == 0) {
          z_vtask_t *oright;
          if ((oright = tmp->link.child[1]))
            oright->flags.rblink = 0;
          tmp->flags.rblink = 1;

          oright = tmp->link.child[1];
          memo = oright->link.child[0];
          if ((tmp->link.child[1] = memo)) {
            memo->link.parent = tmp;
          }

          memo = tmp->link.parent;
          if ((oright->link.parent = memo)) {
            int ichild = (tmp == memo->link.child[0]);
            memo->link.child[ichild] = oright;
          } else {
            *root = (oright);
          }
          oright->link.child[0] = tmp;
          tmp->link.parent = (oright);
          tmp = parent->link.child[0];
        }

        tmp->flags.rblink = parent->flags.rblink;
        parent->flags.rblink = 0;
        if ((memo = tmp->link.child[0]))
          memo->flags.rblink = 0;

        tmp = parent->link.child[0];
        memo = tmp->link.child[1];
        if ((parent->link.child[0] = memo)) {
          memo->link.parent = parent;
        }

        memo = parent->link.parent;
        if ((tmp->link.parent = memo)) {
          int ichild = (parent != memo->link.child[0]);
          memo->link.child[ichild] = tmp;
        } else {
          *root = tmp;
        }
        tmp->link.child[1] = parent;
        parent->link.parent = tmp;
        vtask = *root;
        break;
      }
    }
  }
  if (vtask) {
    vtask->flags.rblink = 0;
  }
}

static z_vtask_t *__vtask_tree_remove(z_vtask_t **root, z_vtask_t *vtask) {
  z_vtask_t *child, *parent, *old = vtask;
  int color;
  if (vtask->link.child[0] == NULL) {
    child = vtask->link.child[1];
  } else if (vtask->link.child[1] == NULL) {
    child = vtask->link.child[0];
  } else {
    z_vtask_t *left;
    z_vtask_t *memo;
    vtask = vtask->link.child[1];
    while ((left = vtask->link.child[0]))
      vtask = left;
    child = vtask->link.child[1];
    parent = vtask->link.parent;
    color = vtask->flags.rblink;
    if (child)
      child->link.parent = parent;
    if (parent) {
      int ichild = (parent->link.child[0] != vtask);
      parent->link.child[ichild] = child;
    } else {
      *root = child;
    }
    if (vtask->link.parent == old)
      parent = vtask;
    vtask->link = old->link;
    if ((memo = old->link.parent)) {
      int ichild = (memo->link.child[0] != old);
      memo->link.child[ichild] = vtask;
    } else {
      *root = vtask;
    }
    old->link.child[0]->link.parent = vtask;
    if ((memo = old->link.child[1]))
      memo->link.parent = vtask;
    goto color;
  }
  parent = vtask->link.parent;
  color = vtask->flags.rblink;
  if (child)
    child->link.parent = parent;
  if (parent) {
    int ichild = (parent->link.child[0] != vtask);
    parent->link.child[ichild] = child;
  } else {
    *root = child;
  }
color:
  if (color == 0)
    __tree_rb_remove_color(root, parent, child);
  return (old);
}

static int __vtask_tree_insert(z_vtask_t **root, z_vtask_t *task) {
  z_vtask_t *parent = NULL;
  z_vtask_t *tmp;
  int is_min = 1;
  long comp = 0;

  tmp = *root;
  while (tmp) {
    parent = tmp;
    comp = __vtask_compare(task, parent);
    Z_ASSERT(comp != 0, "same vtask %p (%"PRIu64")/%p (%"PRIu64")",
                        task, task->seqid, parent, parent->seqid);
    if (comp < 0) {
      tmp = tmp->link.child[0];
    } else {
      tmp = tmp->link.child[1];
      is_min = 0;
    }
  }

  task->link.parent = parent;
  task->link.child[0] = task->link.child[1] = NULL;
  task->flags.rblink = 1;
  if (parent != NULL) {
    parent->link.child[comp >= 0] = task;
  } else {
    *root = task;
  }
  __tree_rb_insert_color(root, task);
  return(is_min);
}

z_vtask_t *z_vtask_tree_next(z_vtask_t *task) {
  if (task->link.child[1]) {
    task = task->link.child[1];
    while (task->link.child[0])
      task = task->link.child[0];
  } else {
    if (task->link.parent && (task == (task->link.parent)->link.child[0])) {
      task = task->link.parent;
    } else {
      while (task->link.parent && (task == (task->link.parent)->link.child[1]))
        task = task->link.parent;
      task = task->link.parent;
    }
  }
  return task;
}

z_vtask_t *z_vtask_tree_prev(z_vtask_t *task) {
  if (task->link.child[0]) {
    task = task->link.child[0];
    while (task->link.child[1])
      task = task->link.child[1];
  } else {
    if (task->link.parent && (task == (task->link.parent)->link.child[1])) {
      task = task->link.parent;
    } else {
      while (task->link.parent && (task == (task->link.parent)->link.child[0]))
        task = task->link.parent;
      task = task->link.parent;
    }
  }
  return task;
}

/* ============================================================================
 *  PUBLIC Task Tree methods
 */
void z_vtask_tree_init (z_vtask_tree_t *self) {
  self->root = NULL;
  self->min_node = NULL;
}

void z_vtask_tree_push (z_vtask_tree_t *self, z_vtask_t *vtask) {
  if (__vtask_tree_insert(&(self->root), vtask))
    self->min_node = vtask;
}

z_vtask_t *z_vtask_tree_pop (z_vtask_tree_t *self) {
  z_vtask_t *vtask = self->min_node;
  self->min_node = z_vtask_tree_next(vtask);
  return(__vtask_tree_remove(&(self->root), vtask));
}
