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

#include <zcl/expr.h>

/* ============================================================================
 *  PRIVATE Expression variables tree
 */
static int __expr_vars_tree_cmp (void *udata, const void *a, const void *b) {
  const z_expr_variable_t *ea = z_container_of(a, const z_expr_variable_t, __node__);
  const z_expr_variable_t *eb = z_container_of(b, const z_expr_variable_t, __node__);
  return((ea == eb) ? 0 : z_bytes_compare(ea->key, eb->key));
}

static int __expr_vars_tree_key_cmp (void *udata, const void *a, const void *key) {
  const z_expr_variable_t *ea = z_container_of(a, const z_expr_variable_t, __node__);
  return(z_byte_slice_compare(&(ea->key->slice), Z_BYTE_SLICE(key)));
}

static const z_tree_info_t __expr_vars_tree_info = {
  .plug         = &z_tree_avl,
  .node_compare = __expr_vars_tree_cmp,
  .key_compare  = __expr_vars_tree_key_cmp,
  .node_free    = NULL,
};

/* ============================================================================
 *  PUBLIC Expression variables
 */
int z_expr_var_add (z_tree_node_t **root, z_expr_variable_t *var) {
  return(z_tree_node_attach(&__expr_vars_tree_info, root, &(var->__node__), NULL));
}

z_expr_variable_t *z_expr_var_pop (z_tree_node_t **root, const z_byte_slice_t *key) {
  z_tree_node_t *node;
  node = z_tree_node_detach(&__expr_vars_tree_info, root, key, NULL);
  return(Z_EXPR_VARIABLE(node));
}

const z_expr_variable_t *z_expr_var_lookup (z_tree_node_t *root,
                                            const z_byte_slice_t *key)
{
  const z_tree_node_t *node;
  node = z_tree_node_lookup(root, __expr_vars_tree_key_cmp, key, NULL);
  return(Z_EXPR_VARIABLE(node));
}
