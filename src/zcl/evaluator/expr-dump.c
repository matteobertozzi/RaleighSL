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

#include <zcl/writer.h>
#include <zcl/expr.h>

/* ============================================================================
 *  PRIVATE Expression Dump
 */
static void __expr_unary_dump (FILE *stream,
                               const z_expr_t *expr,
                               const z_tree_node_t *vars)
{
  const z_expr_unary_t *unary = expr->d.unary;
  switch (unary->type) {
    case Z_EXPR_TOKEN_NOT:          fprintf(stream, "!"); break;
    case Z_EXPR_TOKEN_POSITIVE:     fprintf(stream, "+"); break;
    case Z_EXPR_TOKEN_NEGATIVE:     fprintf(stream, "-"); break;
    case Z_EXPR_TOKEN_BIT_NEGATION: fprintf(stream, "~"); break;
  }
  z_expr_dump(stream, &(unary->operand), vars);
}

static void __expr_binary_dump (FILE *stream,
                                const z_expr_t *expr,
                                const z_tree_node_t *vars)
{
  const z_expr_binary_t *binary = expr->d.binary;
  fprintf(stream, "(");
  z_expr_dump(stream, &(binary->left), vars);
  switch (binary->type) {
    /* Binary Arithmetic */
    case Z_EXPR_TOKEN_ADD:      fprintf(stream, " + ");   break;
    case Z_EXPR_TOKEN_SUB:      fprintf(stream, " - ");   break;
    case Z_EXPR_TOKEN_MUL:      fprintf(stream, " * ");   break;
    case Z_EXPR_TOKEN_DIV:      fprintf(stream, " / ");   break;
    case Z_EXPR_TOKEN_MOD:      fprintf(stream, " %% ");  break;
    case Z_EXPR_TOKEN_LSHIFT:   fprintf(stream, " << ");  break;
    case Z_EXPR_TOKEN_RSHIFT:   fprintf(stream, " >> ");  break;
    case Z_EXPR_TOKEN_BIT_OR:   fprintf(stream, " | ");   break;
    case Z_EXPR_TOKEN_BIT_AND:  fprintf(stream, " & ");   break;
    case Z_EXPR_TOKEN_BIT_XOR:  fprintf(stream, " ^ ");   break;
    /* Binary Comparison */
    case Z_EXPR_TOKEN_EQ:       fprintf(stream, " == ");    break;
    case Z_EXPR_TOKEN_LT:       fprintf(stream, " < ");     break;
    case Z_EXPR_TOKEN_GT:       fprintf(stream, " > ");     break;
    case Z_EXPR_TOKEN_LE:       fprintf(stream, " <= ");    break;
    case Z_EXPR_TOKEN_GE:       fprintf(stream, " >= ");    break;
    case Z_EXPR_TOKEN_NEQ:      fprintf(stream, " != ");    break;
    case Z_EXPR_TOKEN_LIKE:     fprintf(stream, " LIKE ");  break;
    case Z_EXPR_TOKEN_ILIKE:    fprintf(stream, " ILIKE "); break;
    /* Binary Logical */
    case Z_EXPR_TOKEN_OR:       fprintf(stream, " OR ");  break;
    case Z_EXPR_TOKEN_AND:      fprintf(stream, " AND "); break;
  }
  z_expr_dump(stream, &(binary->right), vars);
  fprintf(stream, ")");
}

static void __expr_function_dump (FILE *stream,
                                  const z_expr_t *expr,
                                  const z_tree_node_t *vars)
{
  fprintf(stream, "TODO-FUNCTION(");
  fprintf(stream, ")");
}

/* ============================================================================
 *  PUBLIC Expression Dump
 */
void z_expr_dump (FILE *stream,
                  const z_expr_t *expr,
                  const z_tree_node_t *vars)
{
  switch (expr->type) {
    case Z_EXPR_TYPE_INT:
      fprintf(stream, "%"PRIi64, expr->d.integer);
      break;
    case Z_EXPR_TYPE_FLOAT:
      fprintf(stream, "%.3f", expr->d.floating);
      break;
    case Z_EXPR_TYPE_STRING:
      z_dump_byte_slice(stream, z_bytes_slice(expr->d.string));
      break;
    case Z_EXPR_TYPE_IDENTIFIER:
      z_expr_dump(stream, z_expr_evaluate(expr, vars, NULL), vars);
      break;
    case Z_EXPR_TYPE_UNARY_OP:
      __expr_unary_dump(stream, expr, vars);
      break;
    case Z_EXPR_TYPE_BINARY_OP:
      __expr_binary_dump(stream, expr, vars);
      break;
    case Z_EXPR_TYPE_FUNCTION:
      __expr_function_dump(stream, expr, vars);
      break;
    default:
      fprintf(stream, "Unknown Expression Type %d\n", expr->type);
      break;
  }
}
