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

from collections import deque

import tokenizer

class RqlSyntaxError(Exception):
  pass

class RqlLexer(object):
  __slots__ = ('tokens', 'next_tokens', '_rep_dict')

  def __init__(self, blob):
    self.tokens = iter(tokenizer.tokenize(blob))
    self.next_tokens = deque()
    self._rep_dict = {}

    self._fetch_token()

  def set_replacements(self, rep_dict):
    self._rep_dict = rep_dict

  def has_next(self):
    return len(self.next_tokens) > 0

  def peek(self):
    try:
      return self.next_tokens[0]
    except IndexError:
      return None

  def next(self):
    try:
      return self.next_tokens.popleft()
    except IndexError:
      return None
    finally:
      if not self.next_tokens:
        self._fetch_token()

  def match_seq(self, *utokens):
    match, stack = self._match_seq(*utokens)
    if not match:
      self._reinsert_tokens(stack)
    return match

  def not_match_seq(self, *utokens):
    match, stack = self._match_seq(*utokens)
    if match:
      self._reinsert_tokens(stack)
    return not match

  def peek_seq(self, *utokens):
    match, stack = self._match_seq(*utokens)
    self._reinsert_tokens(stack)
    return match

  def _match_seq(self, *utokens):
    stack = []
    for utoken in utokens:
      token = self.next()
      if not token:
        return False, stack

      stack.append(token)
      if utoken is not None and token.value != utoken:
        return False, stack
    return True, stack

  def peek_one(self, *utokens):
    token = self.peek()
    if token is not None:
      for utoken in utokens:
        if isinstance(utoken, tuple) and self.peek_seq(*utoken):
          return utoken
        if token.value == utoken:
          return utoken
    return None

  def match_one(self, *utokens):
    token = self.peek()
    if token is not None:
      for utoken in utokens:
        if isinstance(utoken, tuple) and self.match_seq(*utoken):
          return utoken
        if token.value == utoken:
          self.next()
          return utoken
    return None

  def match_operator(self, *utokens):
    token = self.peek()
    if token is not None and token.is_operator():
      return self.match_one(*utokens)
    return False

  def expect_one(self, *utokens):
    if not self.match_one(*utokens):
      raise RqlSyntaxError("Expected %r, got %r" % (utokens, self.peek()))

  def expect_seq(self, *utokens):
    if not self.match_seq(*utokens):
      raise RqlSyntaxError("Expected %r, got %r" % (utokens, self.peek()))

  def expect_numeric(self):
    token = self.next()
    if token and token.is_number():
      return token.value
    raise RqlSyntaxError("Expected Numeric, got %r" % token)

  def expect_identifier(self):
    token = self.next()
    if token and token.is_identifier():
      return token.value
    raise RqlSyntaxError("Expected Identifier, got %r" % token)

  def expect_string_or_identifier(self):
    token = self.next()
    if token and (token.is_string() or token.is_identifier()):
      return token.value.lower()
    raise RqlSyntaxError("Expected String or Identifier, got %r" % token)

  def _reinsert_tokens(self, tokens):
    self.next_tokens.extendleft(reversed(tokens))

  def _fetch_token(self):
    try:
      token = self.tokens.next()
      token.value = self._rep_dict.get(token.value, token.value)
      self.next_tokens.append(token)
    except StopIteration:
      pass

if __name__ == '__main__':
  lexer = RqlLexer('a AND b NOT c OR d < k >> l >= b = 2 * 4 == 3')
  while lexer.has_next():
    print lexer.peek()
    print lexer.next()