#!/usr/bin/env python
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

_SYMBOLS_QUOTE = "'\""
_SYMBOLS_SPACE = ' \t\n\r\v'
_SYMBOLS_PUNCTUATION = ',;()'
_SYMBOLS_COMMENT = {'--', '/*'}

TOKEN_NUMBER = 2
TOKEN_BOOLEAN = 3
TOKEN_STRING = 4
TOKEN_IDENTIFIER = 5
TOKEN_PUNCTUATION = 6
TOKEN_OPERATOR = 7
TOKEN_KEYWORD = 8

OPERATORS = {
  '+', '-', '*', '/', '|', '^', '&', '~', '!',
  '=', '<', '>', '<=', '>=', '<>', '!=', '!>', '!<',
}

KEYWORDS_OPERATOR = {'OR', 'AND', 'NOT', 'IN', 'LIKE'}

KEYWORDS = {
  'BEGIN', 'END',
  'SELECT', 'INSERT', 'UPDATE', 'DELETE',
  'CREATE', 'ALTER', 'DROP',
  'FROM',
  'WHERE',
  'ORDER', 'GROUP', 'LIMIT', 'UNION',
}

class Token(object):
  __slots__ = ('type', 'value', 'row', 'col')

  def __init__(self, ttype, tvalue, row, col):
    self.type = ttype
    self.value = tvalue
    self.row = row
    self.col = col

  def __repr__(self):
    return 'Token(%s, %s, row=%d, col=%d)' % (self.type, self.value, self.row, self.col)

  def is_operator(self):
    return self.type == TOKEN_OPERATOR

  def is_identifier(self):
    return self.type == TOKEN_IDENTIFIER

  def is_boolean(self):
    return self.type == TOKEN_BOOLEAN

  def is_number(self):
    return self.type == TOKEN_NUMBER

  def is_string(self):
    return self.type == TOKEN_STRING

def _from_hex(x): return int(x, 16)
def _from_oct(x): return int(x, 8)
def _from_bin(x): return int(x, 2)
TOKEN_NUMBER_FUNC = (float, int, _from_hex, _from_oct, _from_bin)

def _sdata_to_token(sdata, row, col):
  token = ''.join(sdata)
  if '0' <= token[0] <= '9':
    for func in TOKEN_NUMBER_FUNC:
      try:
        value = func(token)
        return Token(TOKEN_NUMBER, value, row, col)
      except ValueError:
        pass

  token = token.upper()
  if token in KEYWORDS_OPERATOR:
    return Token(TOKEN_OPERATOR, token, row, col - 1)
  #if token in KEYWORDS:
  #  return Token(TOKEN_KEYWORD, token, row, col - 1)
  return Token(TOKEN_IDENTIFIER, token, row, col)

def tokenize(query):
  query = iter(query)

  quoted = None
  comment = False
  line_comment = False

  sdata = []
  c = None

  row = 1
  col = 0

  try:
    while True:
      c = next(query)
      col += 1

      if quoted:
        if c == quoted:
          yield Token(TOKEN_STRING, ''.join(sdata), row, col - 1)
          sdata = []
          quoted = None
        else:
          if c == '\\':
            c = next(query)
          sdata.append(c)
        continue

      if comment:
        sdata.append(c)
        if c == '\n':
          row += 1
          col = 0

        if line_comment and c == '\n':
          line_comment = False
          comment = False
          sdata = []
        elif not line_comment and len(sdata) >= 2 and sdata[-2] == '*' and sdata[-1] == '/':
          comment = False
          sdata = []
        continue

      if c in OPERATORS:
        if sdata:
          yield _sdata_to_token(sdata, row, col - len(sdata) - 1)
          sdata = []

        nc = next(query)
        operator = c + nc
        if operator in OPERATORS:
          yield Token(TOKEN_OPERATOR, operator, row, col - 2)
          continue

        if operator in _SYMBOLS_COMMENT:
          line_comment = (operator == '--')
          comment = True
          continue

        yield Token(TOKEN_OPERATOR, c, row, col - 1)
        c = nc

      if c in _SYMBOLS_QUOTE:
        quoted = c
        continue

      if c in _SYMBOLS_SPACE:
        if sdata:
          yield _sdata_to_token(sdata, row, col - len(sdata) - 1)
          sdata = []
        if c == '\n':
          row += 1
          col = 0
        continue

      if c in _SYMBOLS_PUNCTUATION:
        if sdata:
          yield _sdata_to_token(sdata, row, col - len(sdata) - 1)
          sdata = []
        yield Token(TOKEN_PUNCTUATION, c, row, col - 1)
        continue

      sdata.append(c)
  except StopIteration:
    pass

  if quoted:
    raise Exception("Missing end quote")

  if sdata:
    stoken = _sdata_to_token(sdata, row, col - len(sdata))
    yield stoken

if __name__ == '__main__':
  for token in tokenize("(15 + 2) != (- 14 + 3)"):
    print token