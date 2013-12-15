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

#include <zcl/string.h>
#include <zcl/lexer.h>
#include <zcl/debug.h>

#include <ctype.h>

/* ============================================================================
 *  PRIVATE lexer methods
 */
#define __lex_has_more(lex)     ((lex)->index < (lex)->length)
#define __lex_peek(lex)         (__lex_has_more(lex) ? (lex)->query[lex->index] : 0)
#define __lex_next(lex)         ++((lex)->index)

static void __lex_skip_spaces (z_lexer_t *lex) {
  while (__lex_has_more(lex)) {
    if (!isspace(__lex_peek(lex)))
      break;
    __lex_next(lex);
  }
}

static int __lex_number (z_lexer_t *lex) {
  int is_float = 0;
  int mark;
  char c;

  mark = lex->index;
  c = __lex_peek(lex);
  if (!isdigit(c) && c != '.')
    return(0);

  if (c != '.') {
    __lex_next(lex);
    while (isdigit((c = __lex_peek(lex))))
      __lex_next(lex);
  }

  if (c == '.') {
    is_float = 1;
    __lex_next(lex);
    while (isdigit((c = __lex_peek(lex))))
      __lex_next(lex);
  }

  if (c == 'e' || c == 'E') {
    is_float = 1;
    __lex_next(lex);
    c = __lex_peek(lex);
    if (c == '+' || c == '-' || isdigit(c)) {
      __lex_next(lex);
      while (isdigit((c = __lex_peek(lex))))
        __lex_next(lex);
    } else {
      /* Unexpected char after the exponent sign */
      return(-Z_LEXER_UCAES);
    }
  }

  if (lex->index == (mark + 1) && lex->query[mark] == '.') {
    /* Expecting decimal digists after the dot sign */
    return(-Z_LEXER_EDADS);
  }

  return(is_float ? Z_LEXER_TOKEN_FLOAT : Z_LEXER_TOKEN_INT);
}

static int __lex_string (z_lexer_t *lex) {
  char quote;
  char c;

  quote = __lex_peek(lex);
  if (quote != '\'' && quote != '"')
    return(0);

  do {
    __lex_next(lex);
    c = __lex_peek(lex);
    if (c == quote) {
      __lex_next(lex);
      return(Z_LEXER_TOKEN_STRING);
    }

    if (c == '\\') {
      __lex_next(lex);
      if (!__lex_has_more(lex))
        return(-Z_LEXER_UTSTR);

      if (__lex_peek(lex) == quote)
        continue;
    }
  } while (__lex_has_more(lex));

  return(-Z_LEXER_UTSTR);
}

static int __lex_operator_keyword (z_lexer_t *lex, const char *keyword) {
  int mark = lex->index;

  do {
    if (toupper(__lex_peek(lex)) != *keyword++) {
      lex->index = mark;
      return(0);
    }
    __lex_next(lex);
  } while (*keyword != '\0');

  return(1);
}

static int __lex_operator (z_lexer_t *lex) {
  char c, nc;

  c = __lex_peek(lex);
  if (z_memchr("+-*/%~^,;()[]", c, 13) != NULL) {
    __lex_next(lex);
    return(Z_LEXER_TOKEN_OPERATOR);
  }

  /* &, &&, |, || */
  if (z_in(c, '&', '|')) {
    __lex_next(lex);
    if (__lex_peek(lex) == c)
      __lex_next(lex);
    return(Z_LEXER_TOKEN_OPERATOR);
  }

  /* <, <<, <=, >, >>, >=, =, ==, !, !!, !=, <> */
  if (z_memchr("<>=!", c, 4)) {
    __lex_next(lex);
    nc = __lex_peek(lex);
    if (z_in(nc, c, '=') || (c == '<' && nc == '>'))
      __lex_next(lex);
    return(Z_LEXER_TOKEN_OPERATOR);
  }

  if (__lex_operator_keyword(lex, "OR"))
    return(Z_LEXER_TOKEN_OPERATOR);
  if (__lex_operator_keyword(lex, "AND"))
    return(Z_LEXER_TOKEN_OPERATOR);
  if (__lex_operator_keyword(lex, "NOT"))
    return(Z_LEXER_TOKEN_OPERATOR);
  return(0);
}

static int __lex_known_identifiers (const z_lexer_t *lex) {
  const char *svar = lex->query + lex->token.start;
  switch (lex->token.length) {
    case 4:
      if (!z_strncasecmp(svar, "true", 4))
        return(Z_LEXER_TOKEN_BOOLEAN);
      break;
    case 5:
      if (!z_strncasecmp(svar, "false", 5))
        return(Z_LEXER_TOKEN_BOOLEAN);
      break;
  }
  return(Z_LEXER_TOKEN_IDENTIFIER);
}

static int __lex_identifier (z_lexer_t *lex) {
  char c;

  c = __lex_peek(lex);
  if (c != '_' && !isalpha(c))
    return(0);

  __lex_next(lex);
  while (isalnum(__lex_peek(lex)))
    __lex_next(lex);

  return(Z_LEXER_TOKEN_IDENTIFIER);
}

/* ============================================================================
 *  PUBLIC lexer methods
 */
void z_lexer_open (z_lexer_t *lex, const char *query, unsigned int length) {
  lex->query = query;
  lex->length = length;
  lex->index = 0;
  lex->token.type = Z_LEXER_TOKEN_UNKNOWN;
}

const char *z_lexer_error (int code) {
  switch (code) {
    case Z_LEXER_OK:
      return("Lexer Ok");
    case Z_LEXER_UCAES:
      return("Unexpected char after the exponent sign");
    case Z_LEXER_EDADS:
      return("Expecting decimal digists after the dot sign");
    case Z_LEXER_UTSTR:
      return("Unterminated string");
    case Z_LEXER_INVTOK:
      return("Invalid Token");
  }
  return("Unknown error code");
}

int z_lexer_next (z_lexer_t *lex) {
  z_lexer_token_t *token = &(lex->token);
  int state;

  __lex_skip_spaces(lex);
  if (!__lex_has_more(lex)) {
    token->type = Z_LEXER_TOKEN_UNKNOWN;
    return(1);
  }

  token->start = lex->index;
  if ((state = __lex_number(lex))) {
    token->type = state;
  } else if ((state = __lex_string(lex))) {
    token->type = state;
  } else if ((state = __lex_operator(lex))) {
    token->type = state;
  } else if ((state = __lex_identifier(lex))) {
    token->type = state;
  } else {
    token->type = Z_LEXER_TOKEN_UNKNOWN;
    state = -Z_LEXER_INVTOK;
  }
  token->length = (lex->index - token->start);

  if (token->type == Z_LEXER_TOKEN_IDENTIFIER) {
    token->type = __lex_known_identifiers(lex);
  }

  if (state < 0) {
    fprintf(stderr, "Syntax error: %s\n", z_lexer_error(-state));
    return(-1);
  }

  return(0);
}

void z_lexer_token_dump (FILE *stream, const z_lexer_t *lex, const z_lexer_token_t *token) {
  int length = token->length;
  int offset = token->start;

  switch (token->type) {
    case Z_LEXER_TOKEN_UNKNOWN:    fprintf(stream, "UNKNOWN    "); break;
    case Z_LEXER_TOKEN_INT:        fprintf(stream, "INT        "); break;
    case Z_LEXER_TOKEN_FLOAT:      fprintf(stream, "FLOAT      "); break;
    case Z_LEXER_TOKEN_STRING:     fprintf(stream, "STRING     "); break;
    case Z_LEXER_TOKEN_BOOLEAN:    fprintf(stream, "BOOLEAN    "); break;
    case Z_LEXER_TOKEN_OPERATOR:   fprintf(stream, "OPERATOR   "); break;
    case Z_LEXER_TOKEN_IDENTIFIER: fprintf(stream, "IDENTIFIER "); break;
  }

  printf("start=%-2u length=%-2u - ", offset, length);
  while (length--) {
    putc(lex->query[offset++], stream);
  }
}