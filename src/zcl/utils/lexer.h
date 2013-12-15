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

#ifndef _Z_LEXER_H_
#define _Z_LEXER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#include <stdio.h>

Z_TYPEDEF_ENUM(z_lexer_error)
Z_TYPEDEF_ENUM(z_lexer_token_type)
Z_TYPEDEF_STRUCT(z_lexer_token)
Z_TYPEDEF_STRUCT(z_lexer)

enum z_lexer_token_type {
  Z_LEXER_TOKEN_UNKNOWN,
  Z_LEXER_TOKEN_INT,
  Z_LEXER_TOKEN_FLOAT,
  Z_LEXER_TOKEN_STRING,
  Z_LEXER_TOKEN_BOOLEAN,
  Z_LEXER_TOKEN_OPERATOR,
  Z_LEXER_TOKEN_IDENTIFIER,
};

enum z_lexer_error {
  Z_LEXER_OK,
  Z_LEXER_UCAES,    /* Unexpected char after the exponent sign */
  Z_LEXER_EDADS,    /* Expecting decimal digists after the dot sign */
  Z_LEXER_UTSTR,    /* Unterminated String */
  Z_LEXER_INVTOK,   /* Invalid Token */
};

struct z_lexer_token {
  unsigned int start;
  unsigned int length;
  z_lexer_token_type_t type;
};

struct z_lexer {
  const char *query;
  unsigned int length;
  unsigned int index;
  z_lexer_token_t token;
};

#define z_lexer_peek(lex)       (&((lex)->token))

void  z_lexer_open (z_lexer_t *self, const char *query, unsigned int length);
int   z_lexer_next (z_lexer_t *self);

const char *z_lexer_error (int code);

void  z_lexer_token_dump (FILE *stream,
                          const z_lexer_t *lex,
                          const z_lexer_token_t *token);

__Z_END_DECLS__

#endif /* !_Z_LEXER_H_ */
