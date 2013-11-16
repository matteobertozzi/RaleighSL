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

#include <zcl/hashmap.h>

/* ===========================================================================
 *  PRIVATE Open-HashTable macros
 */
#define __entry_hash(self, key)                                               \
  (Z_HASH_MAP(self)->hash(Z_HASH_MAP(self)->udata, key, Z_HASH_MAP(self)->seed))

/* ===========================================================================
 *  PUBLIC Hash-Map methods
 */
void z_hash_map_clear (z_hash_map_t *self) {
  self->plug->clear(self);
  self->size = 0;
}

int z_hash_map_put (z_hash_map_t *self, void *key_value) {
  uint64_t hash = __entry_hash(self, key_value);
  if (self->plug->put(self, hash, key_value)) {
    if (self->plug->resize(self, self->size << 1))
      return(1);
    return(self->plug->put(self, hash, key_value));
  }
  return(0);
}

void *z_hash_map_get (z_hash_map_t *self, const void *key) {
  return(z_hash_map_get_custom(self, self->key_compare, __entry_hash(self, key), key));
}

void *z_hash_map_get_custom (z_hash_map_t *self,
                             z_compare_t key_compare,
                             uint64_t hash,
                             const void *key)
{
  return(self->plug->get(self, key_compare, hash, key));
}

void *z_hash_map_pop (z_hash_map_t *self, const void *key) {
  return(z_hash_map_pop_custom(self, self->key_compare, __entry_hash(self, key), key));
}

void *z_hash_map_pop_custom (z_hash_map_t *self,
                             z_compare_t key_compare,
                             uint64_t hash,
                             const void *key)
{
  return(self->plug->pop(self, key_compare, hash, key));
}

int z_hash_map_remove (z_hash_map_t *self, const void *key) {
  return(z_hash_map_remove_custom(self, self->key_compare, __entry_hash(self, key), key));
}

int z_hash_map_remove_custom (z_hash_map_t *self,
                              z_compare_t key_compare,
                              uint64_t hash,
                              const void *key)
{
  void *data;

  if ((data = z_hash_map_pop_custom(self, key_compare, hash, key)) == NULL)
    return(1);

  if (self->data_free != NULL)
    self->data_free(self->udata, data);
  return(0);
}


/* ===========================================================================
 *  INTERFACE Iterator methods
 */
static int __hash_map_open (void *self, const void *object) {
  z_hash_map_iterator_t *iter = Z_HASH_MAP_ITERATOR(self);
  const z_hash_map_t *map = Z_CONST_HASH_MAP(object);
  Z_ITERATOR_INIT(iter, map);
  return(map->plug->iter_begin(iter, map) == NULL);
}

static void *__hash_map_begin (void *self) {
  z_hash_map_iterator_t *iter = Z_HASH_MAP_ITERATOR(self);
  const z_hash_map_t *map = Z_CONST_HASH_MAP(z_iterator_object(iter));
  return(map->plug->iter_begin(iter, map));
}

static void *__hash_map_end (void *self) {
  z_hash_map_iterator_t *iter = Z_HASH_MAP_ITERATOR(self);
  const z_hash_map_t *map = Z_CONST_HASH_MAP(z_iterator_object(iter));
  return(map->plug->iter_end(iter, map));
}

static void *__hash_map_next (void *self) {
  z_hash_map_iterator_t *iter = Z_HASH_MAP_ITERATOR(self);
  const z_hash_map_t *map = Z_CONST_HASH_MAP(z_iterator_object(iter));
  return(map->plug->iter_next(iter, map));
}

static void *__hash_map_prev (void *self) {
  z_hash_map_iterator_t *iter = Z_HASH_MAP_ITERATOR(self);
  const z_hash_map_t *map = Z_CONST_HASH_MAP(z_iterator_object(iter));
  return(map->plug->iter_prev(iter, map));
}

static void *__hash_map_current (void *self) {
  return(Z_HASH_MAP_ITERATOR(self)->data);
}

/* ===========================================================================
 *  INTERFACE Map methods
 */
static void *__hash_map_get (void *self, const void *key) {
  return(z_hash_map_get(Z_HASH_MAP(self), key));
}

static void *__hash_map_get_custom (void *self, z_compare_t key_compare, const void *key) {
  return(z_hash_map_get_custom(Z_HASH_MAP(self), key_compare, __entry_hash(self, key), key));
}

static int __hash_map_put (void *self, void *key_value) {
  return(z_hash_map_put(Z_HASH_MAP(self), key_value));
}

static void *__hash_map_pop (void *self, const void *key) {
  return(z_hash_map_pop(Z_HASH_MAP(self), key));
}

static void *__hash_map_pop_custom (void *self, z_compare_t key_compare, const void *key) {
  return(z_hash_map_pop_custom(Z_HASH_MAP(self), key_compare, __entry_hash(self, key), key));
}

static int __hash_map_remove (void *self, const void *key) {
  return(z_hash_map_remove(Z_HASH_MAP(self), key));
}

static int __hash_map_remove_custom (void *self, z_compare_t key_compare, const void *key) {
  return(z_hash_map_remove_custom(Z_HASH_MAP(self), key_compare, __entry_hash(self, key), key));
}

static void __hash_map_clear (void *self) {
  z_hash_map_clear(Z_HASH_MAP(self));
}

static int __hash_map_size (const void *self) {
  return(Z_CONST_HASH_MAP(self)->size);
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __hash_map_ctor (void *self, va_list args) {
  z_hash_map_t *map = Z_HASH_MAP(self);
  unsigned int capacity;

  map->plug = va_arg(args, const z_hash_map_plug_t *);
  map->hash = va_arg(args, z_hash_func_t);
  map->key_compare = va_arg(args, z_compare_t);
  map->data_free = va_arg(args, z_mem_free_t);
  map->udata = va_arg(args, void *);
  map->seed = va_arg(args, unsigned int);
  capacity = z_align_up(va_arg(args, unsigned int), 8);
  map->capacity = 0;
  map->buckets = NULL;

  if (map->plug->open != NULL && map->plug->open(map))
    return(1);

  return(map->plug->resize(map, capacity));
}

static void __hash_map_dtor (void *self) {
  z_hash_map_t *map = Z_HASH_MAP(self);
  z_hash_map_clear(map);
  map->plug->close(map);
}

/* ===========================================================================
 *  Hash-Map vtables
 */
static const z_vtable_type_t __hash_map_type = {
  .name = "HashMap",
  .size = sizeof(z_hash_map_t),
  .ctor = __hash_map_ctor,
  .dtor = __hash_map_dtor,
};

static const z_vtable_iterator_t __hash_map_iterator = {
  .open    = __hash_map_open,
  .close   = NULL,
  .begin   = __hash_map_begin,
  .end     = __hash_map_end,
  .skip    = NULL,
  .next    = __hash_map_next,
  .prev    = __hash_map_prev,
  .current = __hash_map_current,
};

static const z_vtable_map_t __hash_map_map = {
  .get            = __hash_map_get,
  .put            = __hash_map_put,
  .pop            = __hash_map_pop,
  .remove         = __hash_map_remove,
  .clear          = __hash_map_clear,
  .size           = __hash_map_size,

  .get_custom     = __hash_map_get_custom,
  .pop_custom     = __hash_map_pop_custom,
  .remove_custom  = __hash_map_remove_custom,
};

static const z_map_interfaces_t __hash_map_interfaces = {
  .type       = &__hash_map_type,
  .iterator   = &__hash_map_iterator,
  .map        = &__hash_map_map,
};

/* ===========================================================================
 *  PUBLIC Hash-Map constructor/destructor
 */
z_hash_map_t *z_hash_map_alloc (z_hash_map_t *self,
                                const z_hash_map_plug_t *plug,
                                z_hash_func_t hash_func,
                                z_compare_t key_compare,
                                z_mem_free_t data_free,
                                void *user_data,
                                unsigned int seed,
                                unsigned capacity)
{
  return(z_object_alloc(self, &__hash_map_interfaces, plug, hash_func,
                        key_compare, data_free, user_data, seed, capacity));
}

void z_hash_map_free (z_hash_map_t *self) {
  z_object_free(self);
}

