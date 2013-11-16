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

#include <zcl/skiplist.h>
#include <zcl/string.h>
#include <zcl/global.h>
#include <zcl/debug.h>
#include <zcl/math.h>

/* ===========================================================================
*  PRIVATE Skip-List macros
*/
#define Z_SKIP_LIST_MAX_HEIGHT      8

#define __skip_node_size(level)                                             \
  (sizeof(z_skip_list_node_t) + ((level) * sizeof(z_skip_list_node_t *)))

#define __skip_head_alloc(skip)                                             \
  __skip_node_alloc(skip, Z_SKIP_LIST_MAX_HEIGHT, NULL)

#define __skip_rand(skip)                                                   \
  z_rand(&((skip)->seed))

/* ===========================================================================
 *  PRIVATE Skip-List data structures
 *  _
 * |_|----------------------_--------->X
 * |_|----------_--------->|_|-------->X
 * |_|----_--->|_|----_--->|_|--- _--->X
 * |_|-->|_|-->|_|-->|_|-->|_|-->|_|-->X
 */
struct z_skip_list_node {
  void *data;
  z_skip_list_node_t *next[0];
};

enum mutation_type {
  SKIP_LIST_INSERT,
  SKIP_LIST_REPLACE,
  SKIP_LIST_DELETE,
  SKIP_LIST_CLEAR,
};

struct mutation {
  struct mutation *next;

  z_skip_list_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT];
  z_skip_list_node_t *p;
  void *key_value;
  unsigned int levels;
  enum mutation_type type;
};

/* ===========================================================================
*  PRIVATE Skip-List Mutation utils
*/
static struct mutation *__mutation_alloc (z_skip_list_t *self) {
  struct mutation *mut;

  mut = z_memory_struct_alloc(z_global_memory(), struct mutation);
  if (Z_MALLOC_IS_NULL(mut))
    return(NULL);

  z_memzero(mut, sizeof(struct mutation));
  return(mut);
}

static void __mutation_free (z_skip_list_t *self, struct mutation *mutation) {
  z_memory_struct_free(z_global_memory(), struct mutation, mutation);
}

static void __mutation_add (z_skip_list_t *self, struct mutation *mutation) {
  struct mutation **entry = (struct mutation **)&(self->mutations);
  while (*entry != NULL)
    entry = &((*entry)->next);
  *entry = mutation;
}

static z_skip_list_node_t *__apply_mutation (const z_skip_list_t *self,
                                             unsigned int level,
                                             z_skip_list_node_t *node) {
  struct mutation *mut = (struct mutation *)self->mutations;
  z_skip_list_node_t *next;

  next = node->next[level];
  for (; mut != NULL; mut = mut->next) {
    switch (mut->type) {
      case SKIP_LIST_REPLACE:
        break;
      case SKIP_LIST_INSERT:
        if (node == mut->prev[level]) {
          next = mut->p;
        }
        break;
      case SKIP_LIST_DELETE:
        if (next == mut->p) {
          next = mut->p->next[level];
        }
        break;
      case SKIP_LIST_CLEAR:
        if (node == &(self->head[level])) {
          next = NULL;
        }
        break;
    }
  }

  return(next);
}

/* ===========================================================================
*  PRIVATE Skip-List utils
*/
static unsigned int __skip_rand_level (z_skip_list_t *skip) {
  unsigned int level = 1;
  while (level < Z_SKIP_LIST_MAX_HEIGHT && (__skip_rand(skip) & 0x3) == 0)
    level++;
  return(level);
}

/* ===========================================================================
*  PRIVATE Skip-List methods
*/
static z_skip_list_node_t *__skip_node_alloc (z_skip_list_t *skip,
                                            unsigned int level,
                                            void *data)
{
  z_skip_list_node_t *node;

  node = z_memory_alloc(z_global_memory(), z_skip_list_node_t, __skip_node_size(level));
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  node->data = data;
  while (level--)
    node->next[level] = NULL;

  return(node);
}

static void __skip_node_free (z_skip_list_t *skip, z_skip_list_node_t *node) {
  if (Z_LIKELY(skip->data_free != NULL && node->data != NULL))
    skip->data_free(skip->user_data, node->data);

  z_memory_free(z_global_memory(), node);
}

static z_skip_list_node_t *__skip_node_min (const z_skip_list_t *self) {
  return(self->head->next[0]);
}

static z_skip_list_node_t *__skip_node_max (const z_skip_list_t *self) {
  z_skip_list_node_t *next;
  z_skip_list_node_t *p;
  unsigned int level;

  p = self->head;
  level = self->levels;
  while (level--) {
    while ((next = p->next[level]) != NULL)
      p = next;
  }

  return((p == self->head) ? NULL : p);
}

enum {
  ITEM_CMP_EQ   = 1,
  ITEM_CMP_LESS = 2,
};

static z_skip_list_node_t *__skip_node_lookup (const z_skip_list_t *self,
                                               unsigned int type,
                                               z_compare_t key_compare,
                                               const void *key,
                                               int *found)
{
  z_skip_list_node_t *next;
  z_skip_list_node_t *p;
  unsigned int level;
  int _found;
  int cmp;

  if (found == NULL)
    found = &_found;

  *found = 0;
  p = self->head;
  level = self->levels;
  while (level--) {
    while ((next = p->next[level]) != NULL) {
      cmp = key_compare(self->user_data, next->data, key);
      if (!cmp && type & ITEM_CMP_EQ) {
        *found = 1;
        return(next);
      }

      if (cmp > 0) {
        if (level == 0 && type & ITEM_CMP_LESS) {
          return(p);
        }
        break;
      }

      p = next;
    }
  }
  return(type & ITEM_CMP_LESS ? p : NULL);
}

#define __skip_node_eq(self, key_compare, key)                                \
  __skip_node_lookup(self, ITEM_CMP_EQ, key_compare, key, NULL)

#define __skip_node_less(self, key_compare, key)                              \
  __skip_node_lookup(self, ITEM_CMP_LESS, key_compare, key, NULL)

#define __skip_node_less_eq(self, key_compare, key, found)                    \
  __skip_node_lookup(self, ITEM_CMP_LESS | ITEM_CMP_EQ, key_compare, key, found)

static z_skip_list_node_t *__skip_node_find (const z_skip_list_t *self,
                                             z_compare_t key_compare,
                                             const void *key,
                                             z_skip_list_node_t **prev,
                                             int *equals)
{
  z_skip_list_node_t *next;
  z_skip_list_node_t *p;
  unsigned int level;
  int cmp = 1;

  p = self->head;
  level = self->levels;
  while (level--) {
    while ((next = __apply_mutation(self, level, p)) != NULL) {
      if ((cmp = key_compare(self->user_data, next->data, key)) >= 0) {
        prev[level] = p;

        if (level == 0) {
          *equals = (cmp == 0);
          return(next);
        }

        break;
      }

      p = next;
    }

    prev[level] = p;
  }

  *equals = 0;
  return(p);
}

/* ===========================================================================
 *  PUBLIC Skip-List WRITE methods helpers
 */
static void __skip_list_commit (z_skip_list_t *self, struct mutation *mut) {
  z_skip_list_node_t **prev = mut->prev;
  z_skip_list_node_t *p = mut->p;
  unsigned int i;

  switch (mut->type) {
    case SKIP_LIST_REPLACE:
      if (self->data_free != NULL && mut->key_value != p->data)
        self->data_free(self->user_data, p->data);
      p->data = mut->key_value;
      break;
    case SKIP_LIST_INSERT:
      if (mut->levels > self->levels) {
        for (i = self->levels; i < mut->levels; ++i) {
          mut->prev[i] = self->head;
        }
        self->levels = mut->levels;
      }
      for (i = 0; i < mut->levels; ++i) {
        prev[i]->next[i] = p;
      }
      self->size++;
      break;
    case SKIP_LIST_DELETE:
      for (i = 0; i < self->levels; ++i) {
        if (prev[i]->next[i] == p) {
          prev[i]->next[i] = p->next[i];
        }
      }
      while (self->levels > 1 && self->head->next[self->levels - 1] == NULL)
        self->levels--;
      self->size--;
      __skip_node_free(self, p);
      break;
    case SKIP_LIST_CLEAR:
      /* TODO */
      break;
  }
}

static int __skip_list_put (z_skip_list_t *self, struct mutation *mut, void *key_value) {
  unsigned int i;
  int equals;

  mut->key_value = key_value;
  mut->p = __skip_node_find(self, self->key_compare, key_value, mut->prev, &equals);
  if (mut->p != NULL && equals) {
    mut->type = SKIP_LIST_REPLACE;
    return(0);
  }

  mut->type = SKIP_LIST_INSERT;
  if ((mut->levels = __skip_rand_level(self)) > self->levels) {
    for (i = self->levels; i < mut->levels; ++i) {
      mut->prev[i] = self->head;
    }
  }

  mut->p = __skip_node_alloc(self, mut->levels, key_value);
  if (Z_MALLOC_IS_NULL(mut->p)) {
    return(1);
  }

  for (i = 0; i < mut->levels; ++i) {
    mut->p->next[i] = __apply_mutation(self, i, mut->prev[i]);
  }

  return(0);
}

static void *__skip_list_remove (z_skip_list_t *self,
                                 struct mutation *mut,
                                 z_compare_t key_compare,
                                 const void *key)
{
  int equals;
  mut->type = SKIP_LIST_DELETE;
  mut->key_value = (void *)key;
  mut->p = __skip_node_find(self, key_compare, key, mut->prev, &equals);
  return((mut->p == NULL || !equals) ? NULL : mut->p->data);
}

/* ===========================================================================
 *  PUBLIC Skip-List WRITE methods
 */
int z_skip_list_put (z_skip_list_t *self, void *key_value) {
  struct mutation *mut;

  mut = __mutation_alloc(self);
  if (Z_MALLOC_IS_NULL(mut))
    return(1);

  if (__skip_list_put(self, mut, key_value)) {
    __mutation_free(self, mut);
    return(2);
  }

  __mutation_add(self, mut);
  return(0);
}

int z_skip_list_put_direct (z_skip_list_t *self, void *key_value) {
  struct mutation mut;
  z_memzero(&mut, sizeof(struct mutation));
  if (__skip_list_put(self, &mut, key_value))
    return(1);
  __skip_list_commit(self, &mut);
  return(0);
}

void *z_skip_list_remove (z_skip_list_t *self, const void *key) {
  return(z_skip_list_remove_custom(self, self->key_compare, key));
}

void *z_skip_list_remove_custom (z_skip_list_t *self, z_compare_t key_compare, const void *key) {
  struct mutation *mut;
  void *data;

  mut = __mutation_alloc(self);
  if (Z_MALLOC_IS_NULL(mut))
    return(NULL);

  if ((data = __skip_list_remove(self, mut, key_compare, key)) == NULL) {
    __mutation_free(self, mut);
    return(NULL);
  }

  __mutation_add(self, mut);
  return(data);
}

void *z_skip_list_remove_direct (z_skip_list_t *self, z_compare_t key_compare, const void *key) {
  struct mutation mut;
  void *data;
  z_memzero(&mut, sizeof(struct mutation));
  if ((data = __skip_list_remove(self, &mut, key_compare, key)) == NULL)
    return(NULL);
  __skip_list_commit(self, &mut);
  return(data);
}

void z_skip_list_clear (z_skip_list_t *self) {
  z_skip_list_node_t *next;
  z_skip_list_node_t *p;

  z_skip_list_rollback(self);

  /* TODO: Mutation */
  for (p = self->head->next[0]; p != NULL; p = next) {
    next = p->next[0];
    __skip_node_free(self, p);
  }

  while (self->levels--) {
    self->head->next[self->levels] = NULL;
  }
  self->levels = 1;
  self->size = 0;
}

void z_skip_list_commit (z_skip_list_t *self) {
  struct mutation *mut = (struct mutation *)self->mutations;

  while (mut != NULL) {
    struct mutation *next = mut->next;
    __skip_list_commit(self, mut);
    __mutation_free(self, mut);
    mut = next;
  }
  self->mutations = NULL;
}

void z_skip_list_rollback (z_skip_list_t *self) {
  struct mutation *mut = (struct mutation *)self->mutations;
  while (mut != NULL) {
    struct mutation *next = mut->next;
    __mutation_free(self, mut);
    mut = next;
  }
  self->mutations = NULL;
}

/* ===========================================================================
 *  PUBLIC Skip-List READ methods
 */
void *z_skip_list_get (const z_skip_list_t *self, const void *key) {
  return(z_skip_list_get_custom(self, self->key_compare, key));
}

void *z_skip_list_get_custom (const z_skip_list_t *self,
                              z_compare_t key_compare,
                              const void *key)
{
  z_skip_list_node_t *node;

  if ((node = __skip_node_eq(self, key_compare, key)) != NULL)
    return(node->data);

  return(NULL);
}

int z_skip_list_size (const z_skip_list_t *self) {
  return(self->size);
}

void *z_skip_list_min (const z_skip_list_t *self) {
  const z_skip_list_node_t *node;

  if ((node = __skip_node_min(self)) != NULL)
    return(node->data);

  return(NULL);
}

void *z_skip_list_max (const z_skip_list_t *self) {
  const z_skip_list_node_t *node;

  if ((node = __skip_node_max(self)) != NULL)
    return(node->data);

  return(NULL);
}

void *z_skip_list_less_eq (const z_skip_list_t *self, const void *key, int *cmp) {
  return(z_skip_list_less_eq_custom(self, self->key_compare, key, cmp));
}

void *z_skip_list_less_eq_custom (const z_skip_list_t *self,
                                  z_compare_t key_compare,
                                  const void *key,
                                  int *found)
{
  z_skip_list_node_t *node;
  node = __skip_node_less_eq(self, key_compare, key, found);
  return((node != NULL) ? node->data : NULL);
}

void *z_skip_list_ceil (const z_skip_list_t *self, const void *key) {
  z_skip_list_node_t *node;
  if ((node = __skip_node_less(self, self->key_compare, key)) == NULL)
    return(NULL);
  node = node->next[0];
  return((node != NULL) ? node->data : NULL);
}

void *z_skip_list_floor (const z_skip_list_t *self, const void *key) {
  z_skip_list_node_t *node;
  if ((node = __skip_node_less(self, self->key_compare, key)) != NULL)
    return(node->data);
  return(NULL);
}

/* ===========================================================================
*  INTERFACE Iterator methods
*/
static int __skip_list_open (void *self, const void *object) {
  z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
  Z_ITERATOR_INIT(iter, Z_CONST_SKIP_LIST(object));
  iter->current = __skip_node_min(Z_CONST_SKIP_LIST(object));
  return(0);
}

static void *__skip_list_begin (void *self) {
  z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
  const z_skip_list_t *skip = Z_CONST_SKIP_LIST(z_iterator_object(iter));
  if ((iter->current = __skip_node_min(skip)) != NULL)
    return(iter->current->data);
  return(NULL);
}

static void *__skip_list_end (void *self) {
  z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
  const z_skip_list_t *skip = Z_CONST_SKIP_LIST(z_iterator_object(iter));
  if ((iter->current = __skip_node_max(skip)) != NULL)
    return(iter->current->data);
  return(NULL);
}

static void *__skip_list_next (void *self) {
  z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
  if (Z_LIKELY(iter->current != NULL)) {
    if ((iter->current = iter->current->next[0]) != NULL)
      return(iter->current->data);
  }
  return(NULL);
}

static void *__skip_list_prev (void *self) {
  z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
  const z_skip_list_t *skip = Z_CONST_SKIP_LIST(z_iterator_object(iter));

  if (iter->current == NULL)
    return(NULL);

  iter->current = __skip_node_less(skip, skip->key_compare, iter->current->data);
  return((iter->current != NULL) ? iter->current->data : NULL);
}

static void *__skip_list_seek (void *self, z_iterator_seek_t seek,
                               z_compare_t key_compare, const void *key)
{
  z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
  const z_skip_list_t *skip = Z_CONST_SKIP_LIST(z_iterator_object(iter));
  switch (seek) {
    case Z_ITERATOR_SEEK_EQ:
      iter->current = __skip_node_eq(skip, key_compare, key);
      break;
    case Z_ITERATOR_SEEK_LT:
      iter->current = __skip_node_less(skip, key_compare, key);
      break;
    case Z_ITERATOR_SEEK_LE:
      iter->current = __skip_node_less_eq(skip, key_compare, key, NULL);
      break;
    case Z_ITERATOR_SEEK_GT:
      iter->current = __skip_node_less_eq(skip, key_compare, key, NULL);
      if (iter->current != NULL) {
        iter->current = iter->current->next[0];
      }
      break;
  }
  return(iter->current != NULL ? iter->current->data : NULL);
}

static void *__skip_list_skip (void *self, long n) {
  void *ret = NULL;
  /* TODO: avoid calling iter functions */
  if (n > 0) {
    while (n-- > 0)
      ret = __skip_list_next(self);
  } else {
    while (n-- > 0)
      ret = __skip_list_prev(self);
  }
  return ret;
}

static void *__skip_list_current (void *self) {
  z_skip_list_iterator_t *iter = Z_SKIP_LIST_ITERATOR(self);
  return((iter->current != NULL) ? iter->current->data : NULL);
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __skip_list_ctor (void *self, va_list args) {
  z_skip_list_t *skip = Z_SKIP_LIST(self);

  skip->head = __skip_head_alloc(skip);
  if (Z_MALLOC_IS_NULL(skip->head))
    return(1);

  skip->key_compare = va_arg(args, z_compare_t);
  skip->data_free = va_arg(args, z_mem_free_t);
  skip->user_data = va_arg(args, void *);
  skip->mutations = NULL;
  skip->seed = va_arg(args, unsigned int);
  skip->levels = 1;
  skip->size = 1;
  return(0);
}

static void __skip_list_dtor (void *self) {
  z_skip_list_t *skip = Z_SKIP_LIST(self);
  z_skip_list_clear(skip);
  __skip_node_free(skip, skip->head);
}

/* ===========================================================================
 *  SkipList vtables
 */
static const z_vtable_type_t __skip_list_type = {
  .name = "SkipList",
  .size = sizeof(z_skip_list_t),
  .ctor = __skip_list_ctor,
  .dtor = __skip_list_dtor,
};

static const z_vtable_iterator_t __skip_list_iterator = {
  .open     = __skip_list_open,
  .close    = NULL,
  .begin    = __skip_list_begin,
  .end      = __skip_list_end,
  .seek     = __skip_list_seek,
  .skip     = __skip_list_skip,
  .next     = __skip_list_next,
  .prev     = __skip_list_prev,
  .current  = __skip_list_current,
};

static const z_iterator_interfaces_t __skip_list_interfaces = {
  .type       = &__skip_list_type,
  .iterator   = &__skip_list_iterator,
};

/* ===========================================================================
 *  PUBLIC Skip-List constructor/destructor
 */
z_skip_list_t *z_skip_list_alloc (z_skip_list_t *self,
                                  z_compare_t key_compare,
                                  z_mem_free_t data_free,
                                  void *user_data,
                                  unsigned int seed)
{
  return(z_object_alloc(self, &__skip_list_interfaces,
                        key_compare, data_free, user_data, seed));
}

void z_skip_list_free (z_skip_list_t *self) {
  z_object_free(self);
}
