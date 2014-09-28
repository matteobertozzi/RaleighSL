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
#include <zcl/array.h>
#include <zcl/sort.h>

#include <string.h>

typedef struct z_array_slot z_array_slot_t;

struct z_array_node {
  z_array_node_t *child[2];
  uint32_t index;
  int8_t   balance;
  uint8_t  __pad;
  uint16_t slots;
  uint8_t  blob[1];
};

struct z_array_slot {
  uint8_t *data;
};

struct avl_insert_data {
  z_array_node_t *parent;
  z_array_node_t *top;
  uint64_t dstack;
};

struct avl_remove_data {
  z_array_node_t *stack[32];    /* Nodes */
  uint8_t dstack[32];           /* Directions moved from stack nodes */
  int istack;                   /* Stack pointer */
};

#define __array_node_slots(node)        ((z_array_slot_t *)((node)->blob))
#define __array_node_slot(node, index)  (&(__array_node_slots(node)[index]))
#define __array_node_data(self, node)   ((node)->blob + (self)->slots_size)

static z_array_node_t *__array_node_alloc (z_array_t *self, uint32_t index) {
  z_array_node_t *node;
  size_t mmsize;

  mmsize  = sizeof(z_array_node_t) - 1;
  mmsize += self->slots_size;
  mmsize += self->node_items * self->isize;

  node = (z_array_node_t *) malloc(mmsize);
  node->child[0] = NULL;
  node->child[1] = NULL;
  node->balance = 0;
  node->index = index;
  node->slots = 1;

  self->length += self->node_items;
  self->node_count++;

  mmsize = self->fanout - 1;
  while (mmsize-- > 0) {
    z_array_slot_t *slot = __array_node_slot(node, mmsize);
    slot->data = NULL;
  }

  return(node);
}

static int __array_slot_alloc (z_array_t *self, z_array_slot_t *slot) {
  slot->data = (uint8_t *) malloc(self->slot_items * self->isize);
  self->length += self->slot_items;
  return(0);
}

static void __array_slot_free (z_array_t *self, z_array_slot_t *slot) {
  free(slot->data);
  self->length -= self->slot_items;
}

static void __array_node_free (z_array_t *self, z_array_node_t *node) {
  uint32_t i;
  for (i = 1; i < node->slots; ++i) {
    __array_slot_free(self, __array_node_slot(node, i - 1));
  }
  free(node);
  self->length -= self->node_items;
}

static void __avl_insert_balance (const struct avl_insert_data *v,
                                  z_array_node_t *node)
{
  z_array_node_t *w;

  do {
    z_array_node_t *p = v->parent;
    uint64_t dstack = v->dstack;
    while (p != node) {
      const int dir = dstack & 1;
      p->balance += (dir ? 1 : -1);
      p = p->child[dir];
      dstack >>= 1;
    }
  } while (0);

  if (v->parent->balance == -2) {
    z_array_node_t *x = v->parent->child[0];
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
    z_array_node_t *x = v->parent->child[1];
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

static void __avl_remove_balance (struct avl_remove_data *v, z_array_node_t *p) {
  int k = v->istack;

  if (p->child[1] == NULL) {
    v->stack[k - 1]->child[v->dstack[k - 1]] = p->child[0];
  } else {
    z_array_node_t *r = p->child[1];
    if (r->child[0] == NULL) {
      r->child[0] = p->child[0];
      r->balance = p->balance;
      v->stack[k - 1]->child[v->dstack[k - 1]] = r;
      v->dstack[k] = 1;
      v->stack[k++] = r;
    } else {
      z_array_node_t *s;
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
    z_array_node_t *y = v->stack[k];

    if (v->dstack[k] == 0) {
      ++(y->balance);
      if (y->balance == +1)
        break;
      if (y->balance == +2) {
        z_array_node_t *x = y->child[1];
        if (x->balance == -1) {
          z_array_node_t *w = x->child[0];
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
        z_array_node_t *x = y->child[0];
        if (x->balance == +1) {
          z_array_node_t *w = x->child[1];
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

static int __array_grow (z_array_t *self, int diff_size) {
  if (self->iroot != NULL) {
    struct avl_insert_data v;
    z_array_node_t *p, *q;
    int k = 0;

    v.top = (z_array_node_t *)&(self->iroot);
    v.parent = self->iroot;
    v.dstack = 0;
    for (q = v.top, p = v.parent; p != NULL; q = p, p = p->child[1]) {
      if (p->balance != 0) {
        v.top = q;
        v.parent = p;
        v.dstack = 0;
        k = 0;
      }
      v.dstack |= (1 << k++);
    }

    while (q->slots < self->fanout && diff_size > 0) {
      z_array_slot_t *slot;

      slot = __array_node_slot(q, q->slots - 1);
      __array_slot_alloc(self, slot);
      diff_size -= self->slots_size;

      q->slots++;
    }

    if (diff_size > 0) {
      z_array_node_t *node;

      node = __array_node_alloc(self, q->index + 1);
      /* TODO: check for allocation errors */

      q->child[1] = node;
      __avl_insert_balance(&v, node);
    }
  } else {
    self->iroot = __array_node_alloc(self, 0);
    return(self->iroot != NULL);
  }
  return(1);
}

static int __array_shrink (z_array_t *self, int diff_size) {
  struct avl_remove_data v;
  z_array_node_t *next;
  z_array_node_t *p;

  if (self->iroot == NULL)
    return(0);

  v.istack = 0;
  p = (z_array_node_t *)(&(self->iroot));
  v.stack[v.istack] = p;
  v.dstack[v.istack++] = 0;
  p = p->child[0];

  while ((next = p->child[1]) != NULL) {
    v.stack[v.istack] = p;
    v.dstack[v.istack++] = 1;
    p = next;
  }

  while (--p->slots > 0 && diff_size > 0) {
    __array_slot_free(self, __array_node_slot(p, p->slots));
    diff_size -= self->slot_items;
  }

  if (diff_size > 0) {
    __avl_remove_balance(&v, p);
    p->child[0] = NULL;
    p->child[1] = NULL;

    __array_node_free(self, p);
  }
  return(0);
}

int z_array_open (z_array_t *self,
                  uint16_t isize,
                  uint16_t fanout,
                  uint32_t sblock,
                  uint32_t capacity)
{
  uint32_t node_blob_size;
  uint32_t slots_size;

  sblock = z_align_up(sblock, 64);
  slots_size = ((fanout - 1) * sizeof(z_array_slot_t));
  node_blob_size = sblock - sizeof(z_array_node_t) - 1;
  Z_ASSERT(slots_size < node_blob_size,
           "slots_size=%u, node_blob_size=%u", slots_size, node_blob_size);
  node_blob_size -= slots_size;

  sblock /= isize;

  self->length = capacity;
  self->lbase  = capacity;
  self->slot_items = sblock;
  self->node_items = (node_blob_size / isize);
  self->iper_block = ((fanout - 1) * sblock) + self->node_items;
  self->isize  = isize;
  self->fanout = fanout;
  self->node_count = 0;
  self->slots_size = slots_size;
  self->ibase  = (capacity > 0) ? (uint8_t *) malloc(capacity * isize) : NULL;
  self->iroot  = NULL;

  return(capacity > 0 ? self->ibase == NULL : 0);
}

static void __array_node_close (z_array_t *self, z_array_node_t *node) {
  if (node->child[0] != NULL) __array_node_close(self, node->child[0]);
  if (node->child[1] != NULL) __array_node_close(self, node->child[1]);

  __array_node_free(self, node);
}

void z_array_close (z_array_t *self) {
  if (self->iroot != NULL) {
    __array_node_close(self, self->iroot);
  }

  if (self->ibase != NULL) {
    free(self->ibase);
  }
}

void z_array_resize (z_array_t *self, uint32_t count) {
  int diff;
  int more = 1;
  if (0 && count < self->length) {
    diff = self->length - count;
    while (diff > 0 && more) {
      more = __array_shrink(self, diff);
      diff = self->length - count;
    }
  } else {
    diff = count - self->length;
    while (diff > 0 && more) {
      more = __array_grow(self, diff);
      diff = count - self->length;
    }
  }
}

void z_array_set_ptr (z_array_t *self, uint32_t index, const void *value) {
  memcpy(z_array_get_ptr(self, index), value, self->isize);
}

static void __array_node_set_all (z_array_t *self,
                                  z_array_node_t *node,
                                  const void *value)
{
  uint32_t i, j;
  uint8_t *p;

  /* setup the node */
  p = __array_node_data(self, node);
  for (i = 0; i < self->node_items; ++i) {
    memcpy(p, value, self->isize);
    p += self->isize;
  }

  /* setup the slots */
  for (j = 1; j < node->slots; ++j) {
    p = __array_node_slot(node, j - 1)->data;
    for (i = 0; i < self->slot_items; ++i) {
      memcpy(p, value, self->isize);
      p += self->isize;
    }
  }

  if (node->child[0] != NULL) {
    __array_node_set_all(self, node->child[0], value);
  }

  if (node->child[1] != NULL) {
    __array_node_set_all(self, node->child[1], value);
  }
}

static void __array_node_zero_all (z_array_t *self, z_array_node_t *node) {
  const size_t slot_size = self->slot_items * self->isize;
  const uint32_t nslots = node->slots - 1;
  uint32_t j;

  /* setup the node */
  memset(__array_node_data(self, node), 0, self->node_items * self->isize);

  /* setup the slots */
  for (j = 0; j < nslots; ++j) {
    memset(__array_node_slot(node, j)->data, 0, slot_size);
  }

  if (node->child[0] != NULL) {
    __array_node_zero_all(self, node->child[0]);
  }

  if (node->child[1] != NULL) {
    __array_node_zero_all(self, node->child[1]);
  }
}

void z_array_set_all (z_array_t *self, const void *value) {
  uint32_t i;

  /* setup base array */
  for (i = 0; i < self->lbase; ++i) {
    memcpy(self->ibase + (i * self->isize), value, self->isize);
  }

  /* setup the tree data */
  if (self->iroot != NULL) {
    __array_node_set_all(self, self->iroot, value);
  }
}

void z_array_zero_all (z_array_t *self) {
  /* setup base array */
  memset(self->ibase, 0, self->lbase * self->isize);

  /* setup the tree data */
  if (self->iroot != NULL) {
    __array_node_zero_all(self, self->iroot);
  }
}

uint8_t *z_array_get_ptr (const z_array_t *self, uint32_t index) {
  z_array_slot_t *slot;
  z_array_node_t *node;
  uint32_t node_index;

  /* lookup from the base-array */
  if (index < self->lbase) {
    return(self->ibase + (index * self->isize));
  }

  index -= self->lbase;
  node_index = index / self->iper_block;

  node = self->iroot;
  while (node != NULL) {
    const int cmp = node->index - node_index;
    if (cmp == 0) break;
    node = node->child[cmp < 0];
  }

  Z_ASSERT(node != NULL, "bad lookup, null node %u", index);
  Z_ASSERT(node->index == node_index, "bad lookup %u != %u", node_index, node->index);

  index -= (node_index * self->iper_block);
  if (index < self->node_items) {
    return(__array_node_data(self, node) + (index * self->isize));
  }

  index -= self->node_items;
  slot = __array_node_slot(node, index / self->slot_items);
  return slot->data + ((index % self->slot_items) * self->isize);
}

/* =============================================================================
 *  Array Iterator
 */
static z_array_node_t *__array_node_lookup_child (z_array_iter_t *self, int child) {
  z_array_node_t *node;

  node = self->array->iroot;
  if (node != NULL) {
    self->height = 0;
    while (node->child[child] != NULL) {
      self->stack[self->height++] = node;
      node = node->child[child];
    }
  }

  self->current = node;
  return(node);
}

static z_array_node_t *__array_node_traverse (z_array_iter_t *self, int dir) {
  z_array_node_t *node;

  if ((node = self->current) == NULL)
    return(NULL);

  if (node->child[dir] != NULL) {
    int ndir = !dir;
    self->stack[self->height++] = node;
    node = node->child[dir];
    while (node->child[ndir] != NULL) {
      self->stack[self->height++] = node;
      node = node->child[ndir];
    }
  } else {
    z_array_node_t *tmp;
    do {
      if (self->height == 0) {
        self->current = NULL;
        return(NULL);
      }

      tmp = node;
      node = self->stack[--(self->height)];
    } while (tmp == node->child[dir]);
  }

  self->current = node;
  return(node);
}

int z_array_iter_init (z_array_iter_t *self, z_array_t *array) {
  self->array = array;

  if (array->lbase > 0) {
    self->current = NULL;
    self->blob = array->ibase;
    self->length = array->lbase;
  } else {
    z_array_node_t *node;
    if ((node = __array_node_lookup_child(self, 0)) != NULL) {
      self->blob = __array_node_data(self->array, node);
      self->length = self->array->node_items;
      self->slot = 0;
    } else {
      self->blob = NULL;
      self->length = 0;
    }
  }
  return(0);
}

int z_array_iter_next (z_array_iter_t *self) {
  z_array_node_t *node;

  if (Z_LIKELY(self->current != NULL)) {
    if (++self->slot < self->current->slots) {
      z_array_slot_t *slot = __array_node_slot(self->current, self->slot - 1);
      self->blob = slot->data;
      self->length = self->array->slot_items;
      return(1);
    }

    node = __array_node_traverse(self, 1);
  } else {
    node = __array_node_lookup_child(self, 0);
  }

  if (node == NULL) {
    self->blob = NULL;
    self->length = 0;
    return(0);
  }

  self->blob = __array_node_data(self->array, node);
  self->length = self->array->node_items;
  self->slot = 0;
  return(1);
}

void z_array_sort (z_array_t *self, z_compare_t cmp_func, void *udata) {
  z_array_iter_t iter;
  //uint8_t buffer[64];
  //uint8_t *pbuffer;
  //z_pqueue_t pq;

  //z_pqueue_open(&pq, NULL, 1 + (self->node_count * self->fanout));

  z_array_iter_init(&iter, self);
  do {
    z_heap_sort(iter.blob, iter.length, self->isize, cmp_func, udata);

    //uint8_t *p = z_pqueue_add_slot(&pq);
    // TODO
  } while (z_array_iter_next(&iter));
#if 0
  z_array_iter_init(&iter, self);
  do {
    uint8_t *p = iter.blob;
    while (iter.length--) {
      z_slice_t *slice = (z_slice_t *)z_pqueue_pop(&pq);
      /* TODO: write item[0] */
      if (--slice->length) {
        z_pqueue_add(&pq, slice, NULL, NULL);
      }

      memcpy(p, slice->data, self->isize);

      p += self->isize;
    }
  } while (z_array_iter_next(&iter));
#endif
}

void z_array_sort_slots (z_array_t *self, z_compare_t cmp_func, void *udata) {
  z_array_iter_t iter;
  z_array_iter_init(&iter, self);
  do {
    z_heap_sort(iter.blob, iter.length, self->isize, cmp_func, udata);
  } while (z_array_iter_next(&iter));
}
