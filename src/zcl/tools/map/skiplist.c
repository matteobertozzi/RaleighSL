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

#define ITEM_CMP_EQ     1
#define ITEM_CMP_LESS   2

#define __skip_node_size(level)                                             \
  (sizeof(z_skip_list_node_t) + ((level) * sizeof(z_skip_list_node_t *)))

#define __skip_head_alloc()                                                 \
  z_skip_node_alloc(Z_SKIP_LIST_MAX_HEIGHT, NULL)

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

/* ===========================================================================
 *  PRIVATE Skip-List node methods
 */
#define __skip_node_min(self)             ((self)->head->next[0])

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

static z_skip_list_node_t *__skip_find_path (const z_skip_list_t *self,
                                             z_compare_t key_compare,
                                             const void *key,
                                             z_skip_list_node_t **prev,
                                             int *equals)
{
  z_skip_list_node_t *next;
  z_skip_list_node_t *p;
  unsigned int level;
  void *udata;
  int cmp = 1;

  p = self->head;
  level = self->levels;
  udata = self->user_data;
  while (level--) {
    while ((next = p->next[level]) != NULL) {
      if ((cmp = key_compare(udata, next->data, key)) >= 0) {
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

static z_skip_list_node_t *__skip_node_lookup (const z_skip_list_t *self,
                                               unsigned int type,
                                               z_compare_t key_compare,
                                               const void *key,
                                               int *found)
{
  z_skip_list_node_t *next;
  z_skip_list_node_t *p;
  unsigned int level;
  void *udata;
  int _found;
  int cmp;

  if (found == NULL)
    found = &_found;

  *found = 0;
  p = self->head;
  level = self->levels;
  udata = self->user_data;
  while (level--) {
    while ((next = p->next[level]) != NULL) {
      cmp = key_compare(udata, next->data, key);
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
  return((type & ITEM_CMP_LESS) ? ((p != self->head) ? p : NULL) : NULL);
}

#define __skip_node_eq(self, key_compare, key)                                \
  __skip_node_lookup(self, ITEM_CMP_EQ, key_compare, key, NULL)

#define __skip_node_less(self, key_compare, key)                              \
  __skip_node_lookup(self, ITEM_CMP_LESS, key_compare, key, NULL)

#define __skip_node_less_eq(self, key_compare, key, found)                    \
  __skip_node_lookup(self, ITEM_CMP_LESS | ITEM_CMP_EQ, key_compare, key, found)

/* ===========================================================================
 *  PUBLIC Skip-List node methods
 */
z_skip_list_node_t *z_skip_node_alloc (unsigned int level, void *data) {
  z_skip_list_node_t *node;

  node = z_memory_alloc(z_global_memory(), z_skip_list_node_t, __skip_node_size(level));
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  node->data = data;
  while (level--) {
    node->next[level] = NULL;
  }
  return(node);
}

void z_skip_node_free (z_skip_list_t *skip, z_skip_list_node_t *node) {
  if (Z_LIKELY(skip->data_free != NULL && node->data != NULL))
    skip->data_free(skip->user_data, node->data);

  z_memory_free(z_global_memory(), node);
}

unsigned int z_skip_node_rand_level (z_skip_list_t *skip) {
  unsigned int level = 1;
  while (level < Z_SKIP_LIST_MAX_HEIGHT && (__skip_rand(skip) & 0x3) == 0)
    ++level;
  return(level);
}

z_skip_list_node_t *z_skip_node_lookup (z_skip_list_t *skip,
                                        z_compare_t key_compare,
                                        const void *key)
{

  return(__skip_node_eq(skip, key_compare, key));
}

z_skip_list_node_t *z_skip_node_less (z_skip_list_t *skip,
                                      z_compare_t key_compare,
                                      const void *key)
{

  return(__skip_node_less(skip, key_compare, key));
}

z_skip_list_node_t *z_skip_node_less_eq (z_skip_list_t *skip,
                                         z_compare_t key_compare,
                                         const void *key,
                                         int *found)
{

  return(__skip_node_less_eq(skip, key_compare, key, found));
}

/* ===========================================================================
 *  PUBLIC Skip-List methods
 */
void z_skip_list_attach (z_skip_list_t *self,
                         unsigned int levels,
                         z_skip_list_node_t *node)
{
  z_skip_list_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT];
  z_skip_list_node_t *p;
  int i, equals;

  Z_ASSERT(node->data != NULL, "Missing data for the node");
  p = __skip_find_path(self, self->key_compare, node->data, prev, &equals);
  if (p != NULL && equals) {
    void *data = node->data;
    node->data = p->data;
    p->data = data;
    z_skip_node_free(self, node);
    return;
  }

  if (levels > self->levels) {
    for (i = self->levels; i < levels; ++i)
      prev[i] = self->head;
    self->levels = levels;
  }

  for (i = 0; i < levels; ++i) {
    node->next[i] = prev[i]->next[i];
    prev[i]->next[i] = node;
  }

  ++self->size;
}

static void __skip_list_detach (z_skip_list_t *self,
                                z_skip_list_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT],
                                z_skip_list_node_t *node)
{
  int i;

  for (i = 0; i < self->levels; ++i) {
    if (prev[i]->next[i] == node)
      prev[i]->next[i] = node->next[i];
  }

  while (self->levels > 1 && self->head->next[self->levels - 1] == NULL) {
    --self->levels;
  }

  --self->size;
}

void z_skip_list_detach (z_skip_list_t *self, z_skip_list_node_t *node) {
  z_skip_list_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT];
  z_skip_list_node_t *p;
  int equals;

  p = __skip_find_path(self, self->key_compare, node->data, prev, &equals);
  Z_ASSERT(p == node && equals, "Node not found p=%p node=%p equals=%d", p, node, equals);

  __skip_list_detach(self, prev, node);
}

int z_skip_list_put (z_skip_list_t *self, void *key_value) {
  z_skip_list_node_t *node;
  unsigned int level;

  level = z_skip_node_rand_level(self);
  node = z_skip_node_alloc(level, key_value);
  if (Z_MALLOC_IS_NULL(node))
    return(1);

  z_skip_list_attach(self, level, node);
  return(0);
}

int z_skip_list_remove_custom (z_skip_list_t *self,
                               z_compare_t key_compare,
                               const void *key)
{
  z_skip_list_node_t *prev[Z_SKIP_LIST_MAX_HEIGHT];
  z_skip_list_node_t *p;
  int equals;

  p = __skip_find_path(self, key_compare, key, prev, &equals);
  if (!equals) return(1);

  __skip_list_detach(self, prev, p);
  z_skip_node_free(self, p);
  return(0);
}

void z_skip_list_clear (z_skip_list_t *self) {
  z_skip_list_node_t *next;
  z_skip_list_node_t *p;

  for (p = self->head->next[0]; p != NULL; p = next) {
    next = p->next[0];
    z_skip_node_free(self, p);
  }

  self->levels = 1;
}

void *z_skip_list_get_custom (z_skip_list_t *self,
                              z_compare_t key_compare,
                              const void *key)
{
  z_skip_list_node_t *node;

  if ((node = z_skip_node_lookup(self, key_compare, key)) != NULL)
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

  skip->head = __skip_head_alloc();
  if (Z_MALLOC_IS_NULL(skip->head))
    return(1);

  skip->key_compare = va_arg(args, z_compare_t);
  skip->data_free = va_arg(args, z_mem_free_t);
  skip->user_data = va_arg(args, void *);
  skip->mutations = NULL;
  skip->seed = va_arg(args, unsigned int);
  skip->levels = 1;
  skip->size = 0;
  return(0);
}

static void __skip_list_dtor (void *self) {
  z_skip_list_t *skip = Z_SKIP_LIST(self);
  z_skip_list_clear(skip);
  z_skip_node_free(skip, skip->head);
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
