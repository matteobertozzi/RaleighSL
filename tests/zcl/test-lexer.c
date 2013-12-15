#include <zcl/string.h>
#include <zcl/lexer.h>

#include <string.h>
#include <stdio.h>

int main (int argc, char **argv) {
  const z_lexer_token_t *token;
  z_lexer_t lex;

  const char *expr = "1 + 2.42 * 3 * (4 / 5) && 1 | b ^ c & a || k AND foo OR jj % 2 + 'foo \\' bar';";
  z_lexer_open(&lex, expr, z_strlen(expr));

  while (z_lexer_next(&lex) == Z_LEXER_OK) {
    token = z_lexer_peek(&lex);
    z_lexer_token_dump(stdout, &lex, token);
    fprintf(stdout, "\n");
  }

  return(0);
}
