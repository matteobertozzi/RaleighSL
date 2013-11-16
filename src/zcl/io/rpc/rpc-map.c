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

#include <zcl/global.h>
#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/hash.h>
#include <zcl/rpc.h>

struct z_rpc_map_node {
  z_rwlock_t lock;
  z_rpc_call_t *entry;
};

/* ============================================================================
 *  PRIVATE Hash-Table helper methods
 */
#define __htable_get_bucket(table, oid)                       \
  ((table)->buckets + (z_hash64a(oid) & ((table)->mask)))

static void __htable_resize (z_rpc_map_t *table, unsigned int required_size) {
  z_memory_t *memory = z_global_memory();
  z_rpc_map_node_t *new_buckets;
  z_rpc_map_node_t *buckets;
  unsigned int new_size;
  unsigned int size;

  new_size = 32;
  while (new_size < required_size)
    new_size <<= 1;

  new_buckets = z_memory_array_alloc(memory, z_rpc_map_node_t, new_size);
  if (Z_MALLOC_IS_NULL(new_buckets))
    return;

  size = new_size * sizeof(z_rpc_map_node_t);
  z_memzero(new_buckets, size);

  size = table->size;
  buckets = table->buckets;
  if (buckets != NULL) {
    unsigned int new_mask = new_size - 1;
    unsigned int i;
    for (i = 0; i < size; ++i) {
      z_rpc_call_t *p = buckets[i].entry;
      while (p != NULL) {
        z_rpc_call_t *next;
        z_rpc_map_node_t *node;

        next = p->hash;
        node = new_buckets + (z_hash64a(p->ctx->req_id) & new_mask);
        p->hash = node->entry;
        node->entry = p;

        p = next;
      }
    }
    z_memory_array_free(memory, buckets);
  }

  table->size = new_size;
  table->mask = new_size - 1;
  table->buckets = new_buckets;
}

static z_rpc_call_t **__htable_find (z_rpc_map_node_t *node, uint64_t oid) {
  z_rpc_call_t **entry = &(node->entry);
  while (*entry && ((*entry)->ctx->req_id != oid)) {
    entry = &((*entry)->hash);
  }
  return(entry);
}

static z_rpc_call_t **__htable_find_entry (z_rpc_map_node_t *node, uint64_t oid) {
  z_rpc_call_t **entry;
  for (entry = &(node->entry); *entry != NULL; entry = &((*entry)->hash)) {
    if ((*entry)->ctx->req_id == oid)
      return(entry);
  }
  return(NULL);
}

/* ============================================================================
 *  PRIVATE Hash-Table methods
 */
static int __htable_open (z_rpc_map_t *table, unsigned int capacity) {
  table->used = 0;
  table->size = 0;
  table->mask = 0;
  table->buckets = NULL;
  z_rwlock_init(&(table->lock));
  __htable_resize(table, capacity);
  return(table->buckets == NULL);
}

static void __htable_close (z_rpc_map_t *table) {
  z_memory_array_free(z_global_memory(), table->buckets);
}

static int __htable_try_insert (z_rpc_map_t *table, z_rpc_call_t *entry) {
  z_read_lock(&(table->lock), z_rwlock, {
    z_rpc_map_node_t *bucket = __htable_get_bucket(table, entry->ctx->req_id);
    z_write_lock(&(bucket->lock), z_rwlock, {
      z_rpc_call_t **node = __htable_find(bucket, entry->ctx->req_id);
      Z_ASSERT((*node) == NULL, "Another entry with oid=%"PRIu64" already in.", (*node)->ctx->req_id);
      entry->hash = NULL;
      *node = entry;
      z_atomic_inc(&(entry->refs));
    });
  });

  if (z_atomic_inc(&(table->used)) >= (table->size << 1)) {
    z_try_write_lock(&(table->lock), z_rwlock, {
      __htable_resize(table, table->used);
    });
  }
  return(0);
}

static z_rpc_call_t *__htable_lookup (z_rpc_map_t *table, uint64_t oid) {
  z_rpc_call_t *entry;

  z_read_lock(&(table->lock), z_rwlock, {
    z_rpc_map_node_t *bucket = __htable_get_bucket(table, oid);
    z_read_lock(&(bucket->lock), z_rwlock, {
      z_rpc_call_t **node = __htable_find_entry(bucket, oid);
      if (node != NULL) {
        entry = *node;
        z_atomic_inc(&(entry->refs));
      } else {
        entry = NULL;
      }
    });
  });

  return(entry);
}

static z_rpc_call_t *__htable_remove (z_rpc_map_t *table, uint64_t oid) {
  z_rpc_call_t *entry;

  z_read_lock(&(table->lock), z_rwlock, {
    z_rpc_map_node_t *bucket = __htable_get_bucket(table, oid);
    z_write_lock(&(bucket->lock), z_rwlock, {
      z_rpc_call_t **node = __htable_find_entry(bucket, oid);
      if (node != NULL) {
        entry = *node;
        *node = entry->hash;
      } else {
        entry = NULL;
      }
    });
  });

  if (entry != NULL)
    z_atomic_dec(&(table->used));
  return(entry);
}

/* ============================================================================
 *  PUBLIC IPC-RPC call methods
 */
z_rpc_call_t *z_rpc_call_alloc (z_rpc_ctx_t *ctx) {
  z_rpc_call_t *self;

  self = z_memory_struct_alloc(z_global_memory(), z_rpc_call_t);
  if (Z_MALLOC_IS_NULL(self))
    return(NULL);

  self->hash = NULL;
  self->ctx = ctx;
  self->refs = 1;
  self->callback = NULL;
  self->udata = NULL;
  return(self);
}

void z_rpc_call_free (z_rpc_call_t *self) {
  if (z_atomic_dec(&(self->refs)) > 0)
    return;

  z_memory_array_free(z_global_memory(), self);
}

/* ============================================================================
 *  PUBLIC IPC-RPC map methods
 */
int z_rpc_map_open (z_rpc_map_t *self) {
  return(__htable_open(self, 32));
}

void z_rpc_map_close (z_rpc_map_t *self) {
  __htable_close(self);
}

z_rpc_call_t *z_rpc_map_add (z_rpc_map_t *self,
                             z_rpc_ctx_t *ctx,
                             z_rpc_callback_t callback,
                             void *ucallback,
                             void *udata)
{
  z_rpc_call_t *call;

  call = z_rpc_call_alloc(ctx);
  if (Z_MALLOC_IS_NULL(call))
    return(NULL);

  call->callback = callback;
  call->ucallback = ucallback;
  call->udata = udata;

  __htable_try_insert(self, call);
  return(call);
}

z_rpc_call_t *z_rpc_map_remove (z_rpc_map_t *self, uint64_t req_id) {
  return(__htable_remove(self, req_id));
}

z_rpc_call_t *z_rpc_map_get (z_rpc_map_t *self, uint64_t req_id) {
  return(__htable_lookup(self, req_id));
}
