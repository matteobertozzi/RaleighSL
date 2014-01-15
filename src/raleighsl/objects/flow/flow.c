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
#include <zcl/debug.h>
#include <zcl/tree.h>

#include "flow.h"

#define RALEIGHSL_FLOW(x)                 Z_CAST(raleighsl_flow_t, x)
#define __FLOW_NODE(x)                    Z_CAST(struct flow_node, x)

struct flow_node {
  z_tree_node_t __node__;

  uint64_t offset;
  z_bytes_ref_t data;
};

typedef struct raleighsl_flow {
  z_tree_node_t *root;
  uint64_t size;
} raleighsl_flow_t;

/* ============================================================================
 *  PRIVATE Flow Node methods
 */
static struct flow_node *__flow_node_alloc (raleighsl_flow_t *flow,
                                            uint64_t offset,
                                            const z_bytes_ref_t *data)
{
  struct flow_node *node;

  node = z_memory_struct_alloc(z_global_memory(), struct flow_node);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  node->offset = offset;
  if (data != NULL) {
    z_bytes_ref_acquire(&(node->data), data);
  } else {
    z_bytes_ref_reset(&(node->data));
  }
  return(node);
}

static void __flow_node_free (void *udata, void *obj) {
  struct flow_node *node = z_container_of(obj, struct flow_node, __node__);
  z_bytes_ref_release(&(node->data));
  z_memory_struct_free(z_global_memory(), struct flow_node, node);
}

static int __flow_node_compare (void *udata, const void *a, const void *b) {
  const struct flow_node *ea = z_container_of(a, const struct flow_node, __node__);
  const struct flow_node *eb = z_container_of(b, const struct flow_node, __node__);
  return((ea == eb) ? 0 : z_cmp(ea->offset, eb->offset));
}

static int __flow_node_key_compare (void *udata, const void *a, const void *key) {
  const struct flow_node *ea = z_container_of(a, const struct flow_node, __node__);
  return(z_cmp(ea->offset, Z_UINT64_PTR_VALUE(key)));
}

/* ============================================================================
 *  PRIVATE Flow Tree methods
 */
static struct z_tree_info __flow_tree_info = {
  .plug         = &z_tree_avl,
  .node_compare = __flow_node_compare,
  .key_compare  = __flow_node_key_compare,
  .node_free    = __flow_node_free,
};

/* ============================================================================
 *  PUBLIC Flow WRITE methods
 */
raleighsl_errno_t raleighsl_flow_append (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         const z_bytes_ref_t *data,
                                         uint64_t *res_size)
{
  raleighsl_flow_t *flow = RALEIGHSL_FLOW(object->membufs);
  struct flow_node *node;

  node = __flow_node_alloc(flow, flow->size, data);
  if (Z_MALLOC_IS_NULL(node))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  /* TODO: Add to staging */
  z_tree_node_attach(&__flow_tree_info, &(flow->root), &(node->__node__), NULL);
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_flow_inject (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         uint64_t offset,
                                         const z_bytes_ref_t *data,
                                         uint64_t *res_size)
{
  return(RALEIGHSL_ERRNO_NOT_IMPLEMENTED);
}

raleighsl_errno_t raleighsl_flow_write (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        uint64_t offset,
                                        const z_bytes_ref_t *data,
                                        uint64_t *res_size)
{
  return(RALEIGHSL_ERRNO_NOT_IMPLEMENTED);
}

raleighsl_errno_t raleighsl_flow_remove (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         uint64_t offset,
                                         uint64_t size,
                                         uint64_t *res_size)
{
  return(RALEIGHSL_ERRNO_NOT_IMPLEMENTED);
}

raleighsl_errno_t raleighsl_flow_truncate (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t size,
                                           uint64_t *res_size)
{
  return(RALEIGHSL_ERRNO_NOT_IMPLEMENTED);
}

/* ============================================================================
 *  PUBLIC Flow READ methods
 */
raleighsl_errno_t raleighsl_flow_read (raleighsl_t *fs,
                                       const raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       uint64_t offset,
                                       uint64_t size,
                                       z_bytes_ref_t *data)
{
  __flow_node_key_compare(NULL, NULL, &offset);
  return(RALEIGHSL_ERRNO_NOT_IMPLEMENTED);
}

/* ============================================================================
 *  Flow Object Plugin
 */
static raleighsl_errno_t __object_create (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_flow_t *flow;

  flow = z_memory_struct_alloc(z_global_memory(), raleighsl_flow_t);
  if (Z_MALLOC_IS_NULL(flow))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  /* TODO */
  flow->size = 0;

  object->membufs = flow;
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_close (raleighsl_t *fs,
                                         raleighsl_object_t *object)
{
  raleighsl_flow_t *flow = RALEIGHSL_FLOW(object->membufs);
  /* TODO: Clear */
  z_memory_struct_free(z_global_memory(), raleighsl_flow_t, flow);
  return(RALEIGHSL_ERRNO_NONE);
}

static void __object_apply (raleighsl_t *fs,
                            raleighsl_object_t *object,
                            raleighsl_txn_atom_t *atom)
{
}

static void __object_revert (raleighsl_t *fs,
                             raleighsl_object_t *object,
                             raleighsl_txn_atom_t *atom)
{
}

static raleighsl_errno_t __object_commit (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  return(RALEIGHSL_ERRNO_NONE);
}

const raleighsl_object_plug_t raleighsl_object_flow = {
  .info = {
    .type = RALEIGHSL_PLUG_TYPE_OBJECT,
    .description = "Flow Object",
    .label       = "flow",
  },

  .create   = __object_create,
  .open     = NULL,
  .close    = __object_close,
  .unlink   = NULL,

  .apply    = __object_apply,
  .revert   = __object_revert,
  .commit   = __object_commit,

  .balance  = NULL,
  .sync     = NULL,
};
