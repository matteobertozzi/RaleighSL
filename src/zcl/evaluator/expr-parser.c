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

#include <zcl/strtol.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/lexer.h>
#include <zcl/array.h>
#include <zcl/expr.h>
#include <zcl/math.h>
#include <zcl/tree.h>

#include <string.h>
#include <stdio.h>

#define __MAX_POOL_SIZE   16

enum expr_parser_error {
  Z_EXPR_OK,
  Z_EXPR_NO_PRIMARY,
  Z_EXPR_UNKNOWN_PRIMARY_TOKEN,
  Z_EXPR_PRIMARY_EXPECTED_PARENTHESIS,
  Z_EXPR_PRIMARY_NOT_WITHOUT_OP,  /* Expected IN or BETWEEN after a NOT */
  Z_EXPR_EXPECTED_END_PARENTHESIS, /* Expected ')' at the end of the function args */
  Z_EXPR_BETWEEN_EXPECTED_AND,
  Z_EXPR_FUNCTION_TOO_MANY_ARGS
};

struct expr_parser {
  const char *error;
  z_lexer_t *lex;

  unsigned int pool_used;
  z_tree_node_t *string_pool;
  z_tree_node_t identifiers[__MAX_POOL_SIZE];
};

static int __parse_expression (struct expr_parser *self, z_expr_t *expr);
static int __parse_predicate (struct expr_parser *self, z_expr_t *expr);

/* ===========================================================================
 *  PRIVATE Task Tree
 */
static int __expr_parser_open (struct expr_parser *self, z_lexer_t *lex) {
  self->error = NULL;
  self->lex = lex;
  self->pool_used = 0;
  self->string_pool = NULL;
  return(0);
}

static void __expr_parser_close (struct expr_parser *self) {
}

static z_bytes_t *__expr_add_identifier (struct expr_parser *self,
                                         const char *identifier,
                                         size_t length)
{
  return z_bytes_from_data(identifier, length);
}

static int __get_binary_precedence(const z_lexer_t *lex, z_expr_binary_type_t *op_type) {
  const z_lexer_token_t *token = z_lexer_peek(lex);
  const char *strop = lex->query + token->start;
  switch (token->length) {
    case 1:
      switch (*strop) {
        case '=': *op_type = Z_EXPR_TOKEN_EQ; return(3);
        case '>': *op_type = Z_EXPR_TOKEN_GT; return(3);
        case '<': *op_type = Z_EXPR_TOKEN_LT; return(3);
        case '+': *op_type = Z_EXPR_TOKEN_ADD; return(4);
        case '-': *op_type = Z_EXPR_TOKEN_SUB; return(4);
        case '|': *op_type = Z_EXPR_TOKEN_BIT_OR;  return(4);
        case '&': *op_type = Z_EXPR_TOKEN_BIT_AND; return(4);
        case '^': *op_type = Z_EXPR_TOKEN_BIT_XOR; return(4);
        case '*': *op_type = Z_EXPR_TOKEN_MUL; return(5);
        case '/': *op_type = Z_EXPR_TOKEN_DIV; return(5);
        case '%': *op_type = Z_EXPR_TOKEN_MOD; return(5);
      }
      break;
    case 2:
      if (z_memeq(strop, "==", 2)) { *op_type = Z_EXPR_TOKEN_EQ;  return(3); }
      if (z_memeq(strop, "!>", 2)) { *op_type = Z_EXPR_TOKEN_LE;  return(3); }
      if (z_memeq(strop, "<=", 2)) { *op_type = Z_EXPR_TOKEN_LE;  return(3); }
      if (z_memeq(strop, ">=", 2)) { *op_type = Z_EXPR_TOKEN_GE;  return(3); }
      if (z_memeq(strop, "!<", 2)) { *op_type = Z_EXPR_TOKEN_GE;  return(3); }
      if (z_memeq(strop, "!=", 2)) { *op_type = Z_EXPR_TOKEN_NEQ; return(3); }
      if (z_memeq(strop, "<>", 2)) { *op_type = Z_EXPR_TOKEN_NEQ; return(3); }
      if (z_memeq(strop, "OR", 2)) { *op_type = Z_EXPR_TOKEN_OR;  return(0); }
      break;
    case 3:
      if (z_memeq(strop, "AND", 3)) { *op_type = Z_EXPR_TOKEN_AND; return(1); }
      break;
    case 4:
      if (z_memeq(strop, "LIKE", 4)) { *op_type = Z_EXPR_TOKEN_LIKE; return(2); }
      break;
    case 5:
      if (z_memeq(strop, "ILIKE", 5)) { *op_type = Z_EXPR_TOKEN_ILIKE; return(2); }
      break;
  }

  //z_lexer_token_dump(stderr, lex, token); fprintf(stderr, "NOT FOUND %s\n", strop);
  return(-1);
}

static int __get_unary_operator(const z_lexer_t *lex, z_expr_unary_type_t *op_type) {
  const z_lexer_token_t *token = z_lexer_peek(lex);
  switch (token->length) {
    case 1:
      switch (lex->query[token->start]) {
        case '-': *op_type = Z_EXPR_TOKEN_NEGATIVE; break;
        case '+': *op_type = Z_EXPR_TOKEN_POSITIVE; break;
        case '~': *op_type = Z_EXPR_TOKEN_BIT_NEGATION; break;
        case '!': *op_type = Z_EXPR_TOKEN_NOT; break;
        default: return(0);
      }
      break;
    case 3:
      if (z_memeq(lex->query + token->start, "NOT", 3)) {
        *op_type = Z_EXPR_TOKEN_NOT;
      } else {
        return(0);
      }
      break;
  }
  return(1);
}

static int __parse_int (struct expr_parser *self, z_expr_t *expr) {
  const z_lexer_token_t *token = z_lexer_peek(self->lex);
  expr->type = Z_EXPR_TYPE_INT;
  z_strtoi64(self->lex->query + token->start, 10, &(expr->d.integer));
  return(Z_EXPR_OK);
}

static int __parse_float (struct expr_parser *self, z_expr_t *expr) {
  const z_lexer_token_t *token = z_lexer_peek(self->lex);
  char *endptr = NULL;

  expr->type = Z_EXPR_TYPE_FLOAT;
  expr->d.floating = strtod(self->lex->query + token->start, &endptr);
  return(Z_EXPR_OK);
}

static int __parse_string (struct expr_parser *self, z_expr_t *expr) {
  expr->type = Z_EXPR_TYPE_STRING;
  return(Z_EXPR_OK);
}

static int __parse_identifier (struct expr_parser *self, z_expr_t *expr) {
  const z_lexer_token_t *token = z_lexer_peek(self->lex);
  z_bytes_t *bytes;
  bytes = __expr_add_identifier(self, self->lex->query + token->start, token->length);
  expr->type = Z_EXPR_TYPE_IDENTIFIER;
  expr->d.string = bytes;
  return(bytes != NULL);
}

static int __parse_function (struct expr_parser *self, z_expr_t *expr) {
  z_expr_function_t *func;
  z_lexer_token_t *token;
  int i = 0;

  token = z_lexer_peek(self->lex);
  if (self->lex->query[token->start] == ')') {
    z_lexer_next(self->lex);
    return(Z_EXPR_OK);
  }

  /* TODO! */
  func = z_expr_function_alloc(Z_EXPR_FUNCTION_CUSTOM);

  expr->type = Z_EXPR_TYPE_FUNCTION;
  expr->d.function = func;

  while (1) {
    if (i >= 4) {
      return(Z_EXPR_FUNCTION_TOO_MANY_ARGS);
    }

    __parse_predicate(self, &(func->args[i++]));

    token = z_lexer_peek(self->lex);
    if (self->lex->query[token->start] != ',')
      break;
    z_lexer_next(self->lex);
  }

  if (self->lex->query[token->start] != ')') {
    return(Z_EXPR_EXPECTED_END_PARENTHESIS);
  }

  z_lexer_next(self->lex);
  return(Z_EXPR_OK);
}

/* a IN (1, 2, 3) -> (a == 1 OR a == 2 OR a == 3) */
static int __parse_in (struct expr_parser *self, z_expr_t *expr, int negate) {
  z_expr_binary_type_t in_type = (negate ? Z_EXPR_TOKEN_NEQ : Z_EXPR_TOKEN_EQ);
  z_expr_binary_t *binary;
  z_lexer_token_t *token;
  z_expr_t value;

  token = z_lexer_peek(self->lex);
  if (self->lex->query[token->start] == '(') {
    z_lexer_next(self->lex);
  }

  z_expr_copy(&value, expr);

  /* a == expr */
  binary = z_expr_binary_alloc(in_type);
  z_expr_copy(&(binary->left), &value);
  __parse_predicate(self, &(binary->right));
  expr->type = Z_EXPR_TYPE_BINARY_OP;
  expr->d.binary = binary;

  while (1) {
    z_expr_binary_t *or_expr;

    token = z_lexer_peek(self->lex);
    if (self->lex->query[token->start] != ',')
      break;

    z_lexer_next(self->lex);

    /* a == expr */
    binary = z_expr_binary_alloc(in_type);
    z_expr_copy(&(binary->left), &value);
    __parse_predicate(self, &(binary->right));

    /* a == expr-1 OR a == expr-2 */
    or_expr = z_expr_binary_alloc(Z_EXPR_TOKEN_OR);
    z_expr_copy(&(or_expr->left), expr);
    or_expr->right.type = Z_EXPR_TYPE_BINARY_OP;
    or_expr->right.d.binary = binary;
    expr->type = Z_EXPR_TYPE_BINARY_OP;
    expr->d.binary = or_expr;
  }

  token = z_lexer_peek(self->lex);
  if (self->lex->query[token->start] == ')') {
    z_lexer_next(self->lex);
  }

  return(Z_EXPR_OK);
}

/*
 * a BETWEEN expr-1 AND expr-2 -> (a >= expr-1 && a <= expr-2)
 * b NOT BETWEEN expr-1 AND expr-2 -> (a < expr-1 || a > expr-2)
 */
static int __parse_between (struct expr_parser *self, z_expr_t *expr, int negate) {
  z_lexer_token_t *token;
  z_expr_binary_t *ge_op;
  z_expr_binary_t *le_op;
  z_expr_binary_t *op;

  ge_op = z_expr_binary_alloc(Z_EXPR_TOKEN_GE);
  z_expr_copy(&(ge_op->left), expr);
  __parse_expression(self, &(ge_op->right));

  token = z_lexer_peek(self->lex);
  if (!(token->length == 3 && z_memeq(self->lex->query + token->start, "AND", 3))) {
    return(Z_EXPR_BETWEEN_EXPECTED_AND);
  }
  z_lexer_next(self->lex);

  le_op = z_expr_binary_alloc(Z_EXPR_TOKEN_LE);
  z_expr_copy(&(le_op->left), expr);
  __parse_expression(self, &(le_op->right));

  op = z_expr_binary_alloc(Z_EXPR_TOKEN_AND);
  op->left.type = Z_EXPR_TYPE_BINARY_OP;
  op->left.d.binary = ge_op;
  op->right.type = Z_EXPR_TYPE_BINARY_OP;
  op->right.d.binary = le_op;

  if (negate) {
    ge_op->type = Z_EXPR_TOKEN_LT;
    le_op->type = Z_EXPR_TOKEN_GT;
    op->type = Z_EXPR_TOKEN_OR;
  }

  expr->type = Z_EXPR_TYPE_BINARY_OP;
  expr->d.binary = op;
  return(Z_EXPR_OK);
}

static int __parse_primary (struct expr_parser *self, z_expr_t *expr) {
  const z_lexer_token_t *token = z_lexer_peek(self->lex);
  int negate = 0;

  switch (token->type) {
    case Z_LEXER_TOKEN_INT:
      __parse_int(self, expr);
      z_lexer_next(self->lex);
      break;
    case Z_LEXER_TOKEN_FLOAT:
      __parse_float(self, expr);
      z_lexer_next(self->lex);
      break;
    case Z_LEXER_TOKEN_BOOLEAN:
      expr->type = Z_EXPR_TYPE_INT;
      expr->d.integer = (token->length == 4);
      z_lexer_next(self->lex);
      break;
    case Z_LEXER_TOKEN_STRING:
      __parse_string(self, expr);
      z_lexer_next(self->lex);
      break;
    case Z_LEXER_TOKEN_IDENTIFIER:
      __parse_identifier(self, expr);
      z_lexer_next(self->lex);
      if (self->lex->query[token->start] == '(') {
        __parse_function(self, expr);
      }
      break;
    case Z_LEXER_TOKEN_OPERATOR:
      if (self->lex->query[token->start] == '(') {
        z_lexer_next(self->lex);
        __parse_predicate(self, expr);
        token = z_lexer_peek(self->lex);
        if (self->lex->query[token->start] != ')') {
          return(Z_EXPR_EXPECTED_END_PARENTHESIS);
        }
        z_lexer_next(self->lex);
      } else {
        return(Z_EXPR_PRIMARY_EXPECTED_PARENTHESIS);
      }
      break;
    default:
      return(Z_EXPR_UNKNOWN_PRIMARY_TOKEN);
  }

  if (expr == NULL) {
    return(Z_EXPR_NO_PRIMARY);
  }

  token = z_lexer_peek(self->lex);
  if (token->length == 3 && z_memeq(self->lex->query + token->start, "NOT", 3)) {
    z_lexer_next(self->lex);
    negate = 1;
    token = z_lexer_peek(self->lex);
  }

  if (token->length == 2 && z_memeq(self->lex->query + token->start, "IN", 2)) {
    z_lexer_next(self->lex);
    return(__parse_in(self, expr, negate));
  } else if (token->length == 7 && z_memeq(self->lex->query + token->start, "BETWEEN", 7)) {
    z_lexer_next(self->lex);
    return(__parse_between(self, expr, negate));
  } else if (negate) {
    return(Z_EXPR_PRIMARY_NOT_WITHOUT_OP);
  }

  return(Z_EXPR_OK);
}

static int __parse_unary (struct expr_parser *self, z_expr_t *expr) {
  const z_lexer_token_t *token = z_lexer_peek(self->lex);
  z_expr_unary_type_t op_type;

  if (token->type == Z_LEXER_TOKEN_OPERATOR && __get_unary_operator(self->lex, &op_type)) {
    z_lexer_next(self->lex);
    expr->type = Z_EXPR_TYPE_UNARY_OP;
    expr->d.unary = z_expr_unary_alloc(op_type);
    return(__parse_unary(self, &(expr->d.unary->operand)));
  }
  return(__parse_primary(self, expr));
}

static int __parse_binary (struct expr_parser *self, z_expr_t *lhs, int min_precedence) {
  z_expr_binary_type_t op_type;
  int prec;

  while ((prec = __get_binary_precedence(self->lex, &op_type)) >= min_precedence) {
    z_expr_binary_t *binary;
    z_lexer_next(self->lex);

    binary = z_expr_binary_alloc(op_type);
    z_expr_copy(&(binary->left), lhs);
    __parse_unary(self, &(binary->right));

    while (__get_binary_precedence(self->lex, &op_type) > prec) {
      __parse_binary(self, &(binary->right), prec);
    }

    lhs->type = Z_EXPR_TYPE_BINARY_OP;
    lhs->d.binary = binary;
  }
  return(Z_EXPR_OK);
}

static int __parse_expression (struct expr_parser *self, z_expr_t *expr) {
  __parse_unary(self, expr);
  return(__parse_binary(self, expr, 3));
}

static int __parse_predicate (struct expr_parser *self, z_expr_t *expr) {
  __parse_unary(self, expr);
  return(__parse_binary(self, expr, 0));
}

int z_expr_parse_predicate (z_lexer_t *lex, z_expr_t *expr) {
  struct expr_parser parser;
  int r;
  __expr_parser_open(&parser, lex);
  r = __parse_predicate(&parser, expr);
  __expr_parser_close(&parser);
  return(r);
}
