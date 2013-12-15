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

#ifndef _Z_EXPR_H_
#define _Z_EXPR_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/lexer.h>
#include <zcl/bytes.h>
#include <zcl/tree.h>

#include <stdio.h>

#define Z_EXPR_VARIABLE(x)              Z_CAST(z_expr_variable_t, x)

Z_TYPEDEF_ENUM(z_expr_function_type)
Z_TYPEDEF_ENUM(z_expr_binary_type)
Z_TYPEDEF_ENUM(z_expr_unary_type)
Z_TYPEDEF_ENUM(z_expr_type)

Z_TYPEDEF_STRUCT(z_expr_variable)
Z_TYPEDEF_STRUCT(z_expr_function)
Z_TYPEDEF_STRUCT(z_expr_binary)
Z_TYPEDEF_STRUCT(z_expr_unary)
Z_TYPEDEF_STRUCT(z_expr)

enum z_expr_type {
  Z_EXPR_TYPE_INT,
  Z_EXPR_TYPE_FLOAT,
  Z_EXPR_TYPE_STRING,
  Z_EXPR_TYPE_IDENTIFIER,
  Z_EXPR_TYPE_UNARY_OP,
  Z_EXPR_TYPE_BINARY_OP,
  Z_EXPR_TYPE_FUNCTION,
};

/* Unary Operations */
enum z_expr_unary_type {
  Z_EXPR_TOKEN_NOT,
  Z_EXPR_TOKEN_POSITIVE,
  Z_EXPR_TOKEN_NEGATIVE,
  Z_EXPR_TOKEN_BIT_NEGATION,
};

/* Binary Operations */
enum z_expr_binary_type {
  /* Binary Arithmetic */
  Z_EXPR_TOKEN_ADD,
  Z_EXPR_TOKEN_SUB,
  Z_EXPR_TOKEN_MUL,
  Z_EXPR_TOKEN_DIV,
  Z_EXPR_TOKEN_MOD,
  Z_EXPR_TOKEN_LSHIFT,
  Z_EXPR_TOKEN_RSHIFT,
  Z_EXPR_TOKEN_BIT_OR,
  Z_EXPR_TOKEN_BIT_AND,
  Z_EXPR_TOKEN_BIT_XOR,
  /* Binary Comparison */
  Z_EXPR_TOKEN_EQ,
  Z_EXPR_TOKEN_LT,
  Z_EXPR_TOKEN_GT,
  Z_EXPR_TOKEN_LE,
  Z_EXPR_TOKEN_GE,
  Z_EXPR_TOKEN_NEQ,
  Z_EXPR_TOKEN_LIKE,
  Z_EXPR_TOKEN_ILIKE,
  /* Binary Logical */
  Z_EXPR_TOKEN_OR,
  Z_EXPR_TOKEN_AND,
};

enum z_expr_function_type {
  Z_EXPR_FUNCTION_MIN,
  Z_EXPR_FUNCTION_MAX,
  Z_EXPR_FUNCTION_CUSTOM,
};

struct z_expr {
  z_expr_type_t type;
  union data {
    int64_t integer;
    double floating;
    z_bytes_t *string;
    z_expr_unary_t *unary;
    z_expr_binary_t *binary;
    z_expr_function_t *function;
  } d;
};

struct z_expr_unary {
  z_expr_unary_type_t type;
  z_expr_t operand;
};

struct z_expr_binary {
  z_expr_binary_type_t type;
  z_expr_t left;
  z_expr_t right;
};

struct z_expr_function {
  z_expr_function_type_t type;
  z_expr_t args[4];
};

struct z_expr_variable {
  __Z_TREE_NODE__
  z_bytes_t *key;
  z_expr_t expr;
};

int z_expr_copy (z_expr_t *dst, const z_expr_t *src);

z_expr_unary_t *   z_expr_unary_alloc    (z_expr_unary_type_t type);
z_expr_binary_t *  z_expr_binary_alloc   (z_expr_binary_type_t type);
z_expr_function_t *z_expr_function_alloc (z_expr_function_type_t type);
void               z_expr_free           (z_expr_t *expr);

int                      z_expr_var_add    (z_tree_node_t **root,
                                            z_expr_variable_t *var);
z_expr_variable_t *      z_expr_var_pop    (z_tree_node_t **root,
                                            const z_byte_slice_t *key);
const z_expr_variable_t *z_expr_var_lookup (const z_tree_node_t *root,
                                            const z_byte_slice_t *key);

const z_expr_t *z_expr_evaluate (const z_expr_t *expr,
                                 const z_tree_node_t *vars,
                                 z_expr_t *result);

int z_expr_parse_predicate (z_lexer_t *lex, z_expr_t *expr);

void z_expr_dump (FILE *stream,
                  const z_expr_t *expr,
                  const z_tree_node_t *vars);

__Z_END_DECLS__

#endif /* !_Z_EXPR_H_ */
