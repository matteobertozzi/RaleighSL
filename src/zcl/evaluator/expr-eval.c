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
#include <zcl/math.h>

#include <string.h>
#include <stdio.h>

/* ============================================================================
 *  Numeric
 */
static const z_expr_t *__eval_numeric_unary_op (z_expr_unary_type_t operation,
                                                const z_expr_t *operand,
                                                z_expr_t *result)
{
  if (operand->type == Z_EXPR_TYPE_INT) {
    result->type = Z_EXPR_TYPE_INT;
    switch (operation) {
      case Z_EXPR_TOKEN_NOT:          result->d.integer = !operand->d.integer; break;
      case Z_EXPR_TOKEN_POSITIVE:     result->d.integer = +operand->d.integer; break;
      case Z_EXPR_TOKEN_NEGATIVE:     result->d.integer = -operand->d.integer; break;
      case Z_EXPR_TOKEN_BIT_NEGATION: result->d.integer = ~operand->d.integer; break;
      default: return(NULL);
    }
  } else {
    result->type = Z_EXPR_TYPE_FLOAT;
    switch (operation) {
      case Z_EXPR_TOKEN_NOT:      result->d.floating = !operand->d.floating; break;
      case Z_EXPR_TOKEN_POSITIVE: result->d.floating = +operand->d.floating; break;
      case Z_EXPR_TOKEN_NEGATIVE: result->d.floating = -operand->d.floating; break;
      default: return(NULL);
    }
  }
  return(result);
}

static const z_expr_t *__eval_integer_binary_op (z_expr_binary_type_t operation,
                                                 long left,
                                                 long right,
                                                 z_expr_t *result)
{
  result->type = Z_EXPR_TYPE_INT;
  switch (operation) {
    /* Binary Arithmetic */
    case Z_EXPR_TOKEN_ADD:     result->d.integer = (left + right);  break;
    case Z_EXPR_TOKEN_SUB:     result->d.integer = (left - right);  break;
    case Z_EXPR_TOKEN_MUL:     result->d.integer = (left * right);  break;
    case Z_EXPR_TOKEN_DIV:     result->d.integer = (left / right);  break;
    case Z_EXPR_TOKEN_MOD:     result->d.integer = (left % right);  break;
    case Z_EXPR_TOKEN_LSHIFT:  result->d.integer = (left >> right); break;
    case Z_EXPR_TOKEN_RSHIFT:  result->d.integer = (left << right); break;
    case Z_EXPR_TOKEN_BIT_OR:  result->d.integer = (left | right);  break;
    case Z_EXPR_TOKEN_BIT_AND: result->d.integer = (left & right);  break;
    case Z_EXPR_TOKEN_BIT_XOR: result->d.integer = (left ^ right);  break;
    /* Binary Comparison */
    case Z_EXPR_TOKEN_EQ:      result->d.integer = (left == right); break;
    case Z_EXPR_TOKEN_LT:      result->d.integer = (left <  right); break;
    case Z_EXPR_TOKEN_GT:      result->d.integer = (left >  right); break;
    case Z_EXPR_TOKEN_LE:      result->d.integer = (left <= right); break;
    case Z_EXPR_TOKEN_GE:      result->d.integer = (left >= right); break;
    case Z_EXPR_TOKEN_NEQ:     result->d.integer = (left != right); break;
    case Z_EXPR_TOKEN_LIKE:    result->d.integer = (left == right); break;
    case Z_EXPR_TOKEN_ILIKE:   result->d.integer = (left == right); break;
    /* Binary Logical */
    case Z_EXPR_TOKEN_OR:      result->d.integer = (left && right); break;
    case Z_EXPR_TOKEN_AND:     result->d.integer = (left || right); break;
    //default: return(NULL);
  }
  return(result);
}

static const z_expr_t *__eval_float_binary_op (z_expr_binary_type_t operation,
                                               double left,
                                               double right,
                                               z_expr_t *result)
{
  result->type = Z_EXPR_TYPE_FLOAT;
  switch (operation) {
    /* Binary Arithmetic */
    case Z_EXPR_TOKEN_ADD: result->d.floating = (left + right); break;
    case Z_EXPR_TOKEN_SUB: result->d.floating = (left - right); break;
    case Z_EXPR_TOKEN_MUL: result->d.floating = (left * right); break;
    case Z_EXPR_TOKEN_DIV: result->d.floating = (left / right); break;
    /* Binary Comparison */
    case Z_EXPR_TOKEN_EQ:  result->d.floating = z_fequals(left, right); break;
    case Z_EXPR_TOKEN_LT:  result->d.floating = (left <  right); break;
    case Z_EXPR_TOKEN_GT:  result->d.floating = (left >  right); break;
    case Z_EXPR_TOKEN_LE:  result->d.floating = (left <= right); break;
    case Z_EXPR_TOKEN_GE:  result->d.floating = (left >= right); break;
    case Z_EXPR_TOKEN_NEQ:   result->d.integer = !z_fequals(left, right); break;
    case Z_EXPR_TOKEN_LIKE:  result->d.integer = z_fequals(left, right); break;
    case Z_EXPR_TOKEN_ILIKE: result->d.integer = z_fequals(left, right); break;
    /* Binary Logical */
    case Z_EXPR_TOKEN_OR:  result->d.floating = (left && right); break;
    case Z_EXPR_TOKEN_AND: result->d.floating = (left || right); break;
    default: return(NULL);
  }
  return(result);
}

static const z_expr_t *__eval_numeric_binary_op (z_expr_binary_type_t operation,
                                                 const z_expr_t *left,
                                                 const z_expr_t *right,
                                                 z_expr_t *result)
{
  switch (((left->type == Z_EXPR_TYPE_INT) << 1) | (right->type == Z_EXPR_TYPE_INT)) {
    case 0: /* left && right NOT INT */
      return(__eval_float_binary_op(operation, left->d.floating, right->d.floating, result));
    case 1: /* left NOT INT */
      return(__eval_float_binary_op(operation, left->d.floating, right->d.integer, result));
    case 2: /* right NOT INT */
      return(__eval_float_binary_op(operation, left->d.integer, right->d.floating, result));
    case 3: /* left && right INT */
      return(__eval_integer_binary_op(operation, left->d.integer, right->d.integer, result));
  }
  /* Never reached */
  return(NULL);
}

/* ============================================================================
 *  String
 */
static const z_expr_t *__eval_string_unary_op (z_expr_unary_type_t operation,
                                               const z_expr_t *operand,
                                               z_expr_t *result)
{
  /* Never reached */
  return(NULL);
}

static const z_expr_t *__eval_string_binary_op (z_expr_binary_type_t operation,
                                                const z_expr_t *left,
                                                const z_expr_t *right,
                                                z_expr_t *result)
{
  /* Never reached */
  return(NULL);
}

static const z_expr_t *__eval_string_number_op (z_expr_binary_type_t operation,
                                                const z_expr_t *string,
                                                const z_expr_t *number,
                                                z_expr_t *result)
{
  /* Never reached */
  return(NULL);
}

/* ============================================================================
 *  PRIVATE Evaluate methods
 */
#define z_expr_is_number(expr)   z_in((expr)->type, Z_EXPR_TYPE_INT, Z_EXPR_TYPE_FLOAT)
#define __expr_is_false(expr)    ((expr)->type == Z_EXPR_TYPE_INT && !((expr)->d.integer))
#define __expr_is_true(expr)     ((expr)->type == Z_EXPR_TYPE_INT && (expr)->d.integer)

static const z_expr_t *__eval_function (const z_expr_function_t *expr,
                                        z_tree_node_t *vars,
                                        z_expr_t *result)
{
  /* Never reached */
  return(NULL);
}

static const z_expr_t *__eval_unary_op (const z_expr_unary_t *expr,
                                        z_tree_node_t *vars,
                                        z_expr_t *result)
{
  const z_expr_t *operand;
  z_expr_t value;

  /* Evaluate operand expressions */
  operand = z_expr_evaluate(&(expr->operand), vars, &value);

  /* Evaluate the operation */
  if (z_expr_is_number(operand))
    return(__eval_numeric_unary_op(expr->type, operand, result));
  return(__eval_string_unary_op(expr->type, operand, result));
}

static const z_expr_t *__eval_binary_op (const z_expr_binary_t *expr,
                                         z_tree_node_t *vars,
                                         z_expr_t *result)
{
  const z_expr_t *right;
  const z_expr_t *left;
  z_expr_t operands[2];

  /* Evaluate left expressions */
  left = z_expr_evaluate(&(expr->left), vars, &(operands[0]));

  /*
   * Avoid to evaluate all the chain if is an AND/OR operation,
   * and we already know the result
   */
  switch (expr->type) {
    case Z_EXPR_TOKEN_OR:
      if (__expr_is_true(left)) {
        result->type = Z_EXPR_TYPE_INT;
        result->d.integer = 1;
        return(result);
      }
      break;
    case Z_EXPR_TOKEN_AND:
      if (__expr_is_false(left)) {
        result->type = Z_EXPR_TYPE_INT;
        result->d.integer = 0;
        return(result);
      }
      break;
    default:
      break;
  }

  /* Evaluate right expressions */
  right = z_expr_evaluate(&(expr->right), vars, &(operands[1]));

  /* Evaluate the operation */
  switch ((z_expr_is_number(left) << 1) | z_expr_is_number(right)) {
    case 0: /* left && right NOT NUMERIC */
      return(__eval_string_binary_op(expr->type, left, right, result));
    case 1: /* left NOT NUMERIC */
      return(__eval_string_number_op(expr->type, left, right, result));
    case 2: /* right NOT NUMERIC */
      return(__eval_string_number_op(expr->type, right, left, result));
    case 3: /* left && right NUMERIC */
      return(__eval_numeric_binary_op(expr->type, left, right, result));
  }

  /* Never reached */
  return(NULL);
}

/* ============================================================================
 *  PUBLIC evaluate methods
 */
const z_expr_t *z_expr_evaluate (const z_expr_t *expr,
                                 z_tree_node_t *vars,
                                 z_expr_t *result)
{
  const z_expr_variable_t *var;

  switch (expr->type) {
    case Z_EXPR_TYPE_IDENTIFIER:
      var = z_expr_var_lookup(vars, &(expr->d.string->slice));
      return(var != NULL ? &(var->expr) : NULL);
    case Z_EXPR_TYPE_UNARY_OP:
      return(__eval_unary_op(expr->d.unary, vars, result));
    case Z_EXPR_TYPE_BINARY_OP:
      return(__eval_binary_op(expr->d.binary, vars, result));
    case Z_EXPR_TYPE_FUNCTION:
      return(__eval_function(expr->d.function, vars, result));
    default:
      break;
  }
  return(expr);
}
