#include <zcl/global.h>
#include <zcl/string.h>
#include <zcl/expr.h>

#include <string.h>
#include <stdio.h>

static void __test_expr (const char *sexpr) {
  z_lexer_t lex;
  z_expr_t expr;
  z_expr_t res;

  z_lexer_open(&lex, sexpr, z_strlen(sexpr));
  z_lexer_next(&lex);

  expr.type = Z_EXPR_TYPE_INT;
  expr.d.integer = -1;

  z_expr_parse_predicate(&lex, &expr);
  fprintf(stdout, "Expr: ");
  z_expr_dump(stdout, &expr, NULL);
  fprintf(stdout, "\n");

  fprintf(stdout, "Result: ");
  z_expr_dump(stdout, z_expr_evaluate(&expr, NULL, &res), NULL);
  fprintf(stdout, "\n");

  z_expr_free(&expr);
  z_expr_free(&res);
}

int main (int argc, char **argv) {
#if 0
  z_expr_variable_t identifiers[6];
  z_tree_node_t *vars = NULL;
#endif
  z_allocator_t allocator;
  //int i;

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  printf("z_expr_function %lu\n", sizeof(z_expr_function_t));
  printf("z_expr_binary %lu\n", sizeof(z_expr_binary_t));
  printf("z_expr_unary %lu\n", sizeof(z_expr_unary_t));
  printf("z_expr_t %lu\n", sizeof(z_expr_t));

  /* Setup vars */
#if 0
  for (i = 0; i < 6; ++i) {
    z_expr_var_add(&vars, &(identifiers[i]));
  }
#endif

  __test_expr("1");
  __test_expr("TRUE");
  __test_expr("(false)");
  __test_expr("(1 + 2)");
  __test_expr("(1 + 3) IN (1, 2, 3)");
  __test_expr("(1 + 3) NOT IN (1, 2, 3)");
  __test_expr("(2 + 1) BETWEEN 0 + 1 AND 3");
  __test_expr("(2 + 1) NOT BETWEEN 0 + 1 AND 3");
  __test_expr("11 * 20 / 3 + 30 ^ 40 / 50");
  __test_expr("11 * 20 / 3 + 30");
  __test_expr("11 * 21 / 2");

  z_global_context_close();
  z_allocator_close(&allocator);
  return(0);
}
