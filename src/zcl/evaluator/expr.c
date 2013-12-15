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
#include <zcl/expr.h>

z_expr_unary_t *z_expr_unary_alloc (z_expr_unary_type_t type) {
  z_expr_unary_t *expr;

  expr = z_memory_struct_alloc(z_global_memory(), z_expr_unary_t);
  if (Z_MALLOC_IS_NULL(expr))
    return(NULL);

  expr->type = type;
  return(expr);
}

z_expr_binary_t *z_expr_binary_alloc (z_expr_binary_type_t type) {
  z_expr_binary_t *expr;

  expr = z_memory_struct_alloc(z_global_memory(), z_expr_binary_t);
  if (Z_MALLOC_IS_NULL(expr))
    return(NULL);

  expr->type = type;
  return(expr);
}

z_expr_function_t * z_expr_function_alloc (z_expr_function_type_t type) {
  z_expr_function_t *expr;

  expr = z_memory_struct_alloc(z_global_memory(), z_expr_function_t);
  if (Z_MALLOC_IS_NULL(expr))
    return(NULL);

  expr->type = type;
  expr->args[0].type = Z_EXPR_TYPE_INT;
  expr->args[1].type = Z_EXPR_TYPE_INT;
  expr->args[2].type = Z_EXPR_TYPE_INT;
  expr->args[3].type = Z_EXPR_TYPE_INT;
  return(expr);
}

static void __expr_free (z_memory_t *memory, z_expr_t *expr) {
  switch (expr->type) {
    case Z_EXPR_TYPE_STRING:
      break;
    case Z_EXPR_TYPE_IDENTIFIER:
      break;
    case Z_EXPR_TYPE_UNARY_OP:
      __expr_free(memory, &(expr->d.unary->operand));
      z_memory_struct_free(memory, z_expr_unary_t, expr->d.unary);
      break;
    case Z_EXPR_TYPE_BINARY_OP:
      __expr_free(memory, &(expr->d.binary->left));
      __expr_free(memory, &(expr->d.binary->right));
      z_memory_struct_free(memory, z_expr_binary_t, expr->d.binary);
      break;
    case Z_EXPR_TYPE_FUNCTION:
      z_memory_struct_free(memory, z_expr_function_t, expr->d.function);
      break;
    default:
      break;
  }
}

void z_expr_free (z_expr_t *expr) {
  __expr_free(z_global_memory(), expr);
}

int z_expr_copy (z_expr_t *dst, const z_expr_t *src) {
  int r = 0;
  switch ((dst->type = src->type)) {
    case Z_EXPR_TYPE_INT:
      dst->d.integer = src->d.integer;
      break;
    case Z_EXPR_TYPE_FLOAT:
      dst->d.floating = src->d.floating;
      break;
    case Z_EXPR_TYPE_STRING:
    case Z_EXPR_TYPE_IDENTIFIER:
      dst->d.string = z_bytes_acquire(src->d.string);
      break;
    case Z_EXPR_TYPE_UNARY_OP:
      dst->d.unary = z_expr_unary_alloc(src->d.unary->type);
      if (Z_MALLOC_IS_NULL(dst->d.unary)) return(1);
      r |= z_expr_copy(&(dst->d.unary->operand), &(src->d.unary->operand));
      break;
    case Z_EXPR_TYPE_BINARY_OP:
      dst->d.binary = z_expr_binary_alloc(src->d.binary->type);
      if (Z_MALLOC_IS_NULL(dst->d.binary)) return(1);
      r |= z_expr_copy(&(dst->d.binary->left), &(src->d.binary->left));
      r |= z_expr_copy(&(dst->d.binary->right), &(src->d.binary->right));
      break;
    case Z_EXPR_TYPE_FUNCTION:
      dst->d.function = z_expr_function_alloc(src->d.function->type);
      if (Z_MALLOC_IS_NULL(dst->d.function)) return(1);
      r |= z_expr_copy(&(dst->d.function->args[0]), &(src->d.function->args[0]));
      r |= z_expr_copy(&(dst->d.function->args[1]), &(src->d.function->args[1]));
      r |= z_expr_copy(&(dst->d.function->args[2]), &(src->d.function->args[2]));
      r |= z_expr_copy(&(dst->d.function->args[3]), &(src->d.function->args[3]));
      break;
    default:
      break;
  }
  return(r);
}
