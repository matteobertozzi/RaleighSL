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

from lexer import RqlLexer, RqlSyntaxError

import random
import math

DEFAULT_CONSTANTS = {
  'PI': 3.1415926535897932384,
  'NULL': None,
  'FALSE': False,
  'TRUE': True,
}

DEFAULT_FUNCTIONS = {
  'abs': abs,
  'min': min,
  'max': max,
  'sum': lambda *args: sum(args),
  'acos': math.acos,
  'asin': math.asin,
  'atan': math.atan,
  'ceil': math.ceil,
  'cos': math.cos,
  'exp': math.exp,
  'floor': math.floor,
  'log': math.log,
  'random': random.random,
  'sin': math.sin,
  'sqrt': math.sqrt,
  'tan': math.tan
}

# http://technet.microsoft.com/en-us/library/ms190276.aspx
EXPRESSION_OPERATORS = [
  {'+', '-', '&', '^', '|'},
  {'*', '/', '%'},
  {'~'},
]

PREDICATE_OPERATORS = [
  {'OR'},
  {'AND'},
  {'LIKE'},
  {'==', '<', '>', '<=', '>=', '<>', '==', '!=', '!>', '!<'},
]

PREDICATE_EXPRESSION_OPERATORS = PREDICATE_OPERATORS + EXPRESSION_OPERATORS

def get_binary_precedence(operators, token):
  if token:
    tvalue = token.value
    precedence = 0
    for oplist in operators:
      if tvalue in oplist:
        return precedence
      precedence += 1
  return -1

def get_expr_identifiers(expression):
  identifiers = set()
  for key in expression.__slots__:
    expr = getattr(expression, key)
    if isinstance(expr, RqlIdentifier):
      identifiers.add(expr)
    else:
      identifiers |= get_identifiers(expression)
  return identifiers

class RqlFunctionCall(object):
  __slots__ = ('name', 'args')

  def __init__(self, name, args):
    self.name = name
    self.args = args

  def __str__(self):
    return '%s(%s)' % (self.name, self.args)

  def __repr__(self):
    return 'RqlFunctionCall(%r, %r)' % (self.name, self.args)

  def is_constant(self, context ):
    return False

  def resolve(self, context):
    # TODO: If args are const
    return self

  def evaluate(self, context):
    func = context.functions.get(self.name)
    if not func:
      raise RqlSyntaxError("Unknown function '%s'" % self.name)

    args = []
    for arg in self.args:
      args.append(arg.evaluate(context))
    return func(*args)

class RqlIdentifier(object):
  __slots__ = ('name')

  def __init__(self, name):
    self.name = name

  def __str__(self):
    return self.name

  def __repr__(self):
    return 'RqlIdentifier(%r)' % (self.name)

  def __cmp__(self, other):
    return cmp(self.name, other)

  def __hash__(self):
    return hash(self.name)

  def is_constant(self, context):
    return False

  def resolve(self, context):
    return self

  def evaluate(self, context):
    return context.get_identifier(self.name)

class RqlBaseDataType(object):
  __slots__ = ('value')

  def __init__(self, value):
    self.value = value

  def __str__(self):
    return '%s' % self.value

  def __repr__(self):
    return '%s(%r)' % (self.__class__.__name__, self.value)

  def is_constant(self, context):
    return True

  def resolve(self, context):
    return self

  def evaluate(self, context):
    return self.value

class RqlNumber(RqlBaseDataType):
  def __init__(self, value):
    super(RqlNumber, self).__init__(float(value))

class RqlBoolean(RqlBaseDataType):
  def __init__(self, value):
    super(RqlBoolean, self).__init__(bool(value))

class RqlString(RqlBaseDataType):
  def __init__(self, value):
    super(RqlString, self).__init__(value)

  def __str__(self):
    return '"%s"' % self.value

class RqlWildcard(RqlBaseDataType):
  def __init__(self, value):
    super(RqlWildcard, self).__init__(value)

class RqlUnary(object):
  __slots__ = ('operator', 'expression')

  def __init__(self, operator, expression):
    self.operator = operator
    self.expression = expression

  def __str__(self):
    return '%s %s' % (self.operator, self.expression)

  def __repr__(self):
    return 'RqlUnary(%r, %r)' % (self.operator, self.expression)

  def is_constant(self, context):
    return self.expression.is_constant(context)

  def resolve(self, context):
    self.expression = self.expression.resolve(context)
    if self.is_constant(context):
      return RqlNumber(self.evaluate(context))
    return self

  def evaluate(self, context):
    expr = self.expression.evaluate(context)
    if self.operator == '+': return expr
    if self.operator == '-': return -expr
    if self.operator == '~': return ~expr
    if self.operator == 'NOT': return not expr
    raise RqlSyntaxError("Unknown operator '%s'" % self.operator)

class RqlBinary(object):
  __slots__ = ('operator', 'left', 'right')

  def __init__(self, operator, left, right):
    self.operator = operator
    self.left = left
    self.right = right

  def __str__(self):
    return '(%s %s %s)' % (self.left, self.operator, self.right)

  def __repr__(self):
    return 'RqlBinary(%r, %r, %r)' % (self.operator, self.left, self.right)

  def is_constant(self, context):
    return self.left.is_constant(context) and self.right.is_constant(context)

  def resolve(self, context):
    self.left = self.left.resolve(context)
    self.right = self.right.resolve(context)
    if self.is_constant(context):
      result = self.evaluate(context)
      if isinstance(result, basestring):
        return RqlString(result)
      if isinstance(result, (int, float)):
        return RqlNumber(result)
      if isinstance(result, bool):
        return RqlBoolean(result)
      raise RqlSyntaxError("Unexpected type %s %r" % (type(result), result))
    return self

  def evaluate(self, context):
    left = self.left.evaluate(context)
    right = self.right.evaluate(context)

    # Expression
    if self.operator == '+': return left + right
    if self.operator == '-': return left - right
    if self.operator == '&': return left & right
    if self.operator == '|': return left | right
    if self.operator == '^': return left ^ right
    if self.operator == '*': return left * right
    if self.operator == '/': return left / right
    if self.operator == '%': return left % right

    # Predicate
    if self.operator == '=':   return left == right
    if self.operator == '<':   return left < right
    if self.operator == '>':   return left > right
    if self.operator == '<=':  return left <= right
    if self.operator == '>=':  return left >= right
    if self.operator == '==':  return left == right
    if self.operator == '!=':  return left != right
    if self.operator == 'IS':  return left is right
    if self.operator == 'OR':  return left or right
    if self.operator == 'AND': return left and right
    # LIKE

    raise RqlSyntaxError("Unknown operator '%s'" % self)

class RqlAssignment(object):
  __slots__ = ('name', 'expression')

  def __init__(self, name, expression):
    self.name = name
    self.expression = expression

  def __repr__(self):
    return 'RqlAssignment(%r, %r)' % (self.name, self.expression)

  def is_constant(self, context):
    return self.expression.is_constant(context)

  def resolve(self):
    self.expression = self.expression.resolve()
    if self.is_constant(context):
      return RqlNumber(self.evaluate(context))
    return self

  def evaluate(self, context):
    right = self.expression.evaluate(context)
    context.variables[self.name] = right
    return right

class RqlParser(object):
  __slots__ = ('lexer')

  def __init__(self, lexer):
    self.lexer = lexer

  def parse_identifier(self):
    items = self.parse_dot_list(self.lexer.expect_string_or_identifier)
    return RqlIdentifier('.'.join(items))

  # <List> ::= <Item> '.' <List> | <Item>
  def parse_dot_list(self, item_parse_func):
    return self.parse_list('.', item_parse_func)

  # <List> ::= <Item> ',' <List> | <Item>
  def parse_comma_list(self, item_parse_func):
    return self.parse_list(',', item_parse_func)

  # <List> ::= <Item> <separator> <List> | <Item>
  def parse_list(self, separator, item_parse_func):
    def_list = []
    while True:
      item_def = item_parse_func()
      if not item_def:
        raise Exception("Invalid list item definition")

      def_list.append(item_def)
      if not self.lexer.match_seq(separator):
        break
    return def_list

  def _call_method(self, name, *args, **kwargs):
    return getattr(self, name)(*args, **kwargs)

# =============================================================================
#  Expression
# =============================================================================
class RqlExprParser(RqlParser):
  # FunctionCall ::= Identifier '(' ')' ||
  #                  Identifier '(' ArgumentList ')'
  def parse_function_call(self):
    args = []
    name = self.lexer.expect_identifier().lower()
    self.lexer.expect_seq('(')
    if not self.lexer.peek_seq(')'):
      args = self.parse_comma_list(self.parse_expression)
    self.lexer.expect_seq(')')
    return RqlFunctionCall(name, args)

  # Primary ::= Identifier |
  #             Number |
  #             '(' Assignment ')' |
  #             FunctionCall
  def parse_primary(self):
    token = self.lexer.peek()
    if not token:
      raise RqlSyntaxError("Unexpected termination of expression")

    if self.lexer.match_seq('('):
      expr = self.parse_assignment()
      self.lexer.expect_seq(')')
    elif token.is_identifier():
      if self.lexer.peek_seq(None, '('):
        return self.parse_function_call()
      expr = self.parse_identifier()
    elif token.is_number():
      token = self.lexer.next()
      expr = RqlNumber(token.value)
    elif token.is_string():
      token = self.lexer.next()
      expr = RqlString(token.value)
    elif token.is_boolean():
      token = self.lexer.next()
      expr = RqlBoolean(token.value)
    elif token.value == '*':
      token = self.lexer.next()
      expr = RqlWildcard(token.value)
    else:
      raise RqlSyntaxError("Parse error, can not process token %r" % token)

    if self.lexer.match_seq('BETWEEN'):
      return self.parse_between(expr)
    elif self.lexer.match_seq('NOT', 'BETWEEN'):
      return RqlUnary('NOT', self.parse_between(expr))
    return expr

  # Unary ::= Primary |
  #           '-' Unary |
  #           '~' Unary
  def parse_unary(self):
    operator = self.lexer.match_operator('-', '+', '~', 'NOT')
    if operator:
      expr = self.parse_unary()
      return RqlUnary(operator, expr)
    return self.parse_primary()

  def parse_between(self, expr):
    # expr >= min_expr AND expr <= max_expr
    min_expr = self.parse_expression()
    self.lexer.expect_seq('AND')
    max_expr = self.parse_expression()
    return RqlBinary('AND', RqlBinary('>=', expr, min_expr), RqlBinary('<=', expr, min_expr))

  def parse_binary(self, operators, lhs, min_precedence):
    while get_binary_precedence(operators, self.lexer.peek()) >= min_precedence:
      operation = self.lexer.next()
      rhs = self.parse_unary()

      in_precedence = min_precedence
      prec = get_binary_precedence(operators, self.lexer.peek())
      while prec > in_precedence:
        rhs = self.parse_binary(operators, rhs, prec)
        in_precedence = prec
        prec = get_binary_precedence(operators, self.lexer.peek())
      lhs = RqlBinary(operation.value, lhs, rhs)
    return lhs

  def parse_expression(self):
    return self.parse_binary(EXPRESSION_OPERATORS, self.parse_unary(), 0)

  def parse_predicate(self):
    return self.parse_binary(PREDICATE_EXPRESSION_OPERATORS, self.parse_unary(), 0)

  # Assignment ::= Identifier '=' Assignment |
  #                Additive
  def parse_assignment(self):
    expr = self.parse_binary(PREDICATE_EXPRESSION_OPERATORS, self.parse_unary(), 0)
    if isinstance(expr, RqlIdentifier):
      if self.lexer.match_seq('='):
        return RqlAssignment(expr, self.parse_assignment())
      return expr
    return expr

  @staticmethod
  def parse(expression):
    lexer = RqlLexer(expression)
    parser = RqlExprParser(lexer)
    expr = parser.parse_assignment()
    if lexer.has_next():
      raise RqlSyntaxError("Unexpected token '%s'" % lexer.peek())
    return expr

class RqlContext:
  NULL = object()

  def __init__(self):
    self.constants = dict(DEFAULT_CONSTANTS)
    self.functions = dict(DEFAULT_FUNCTIONS)
    self.variables = {}

  def get_identifier(self, name):
    value = self.constants.get(name.upper(), self.NULL)
    if value is not self.NULL: return value

    value = self.variables.get(name.lower(), self.NULL)
    if value is not self.NULL: return value

    raise RqlSyntaxError("Unknown identifier '%s' - %r" % (name, self.variables))

  def get_qualified_identifier(self, qualifier, name):
    return self.get_identifier('%s.%s' % (qualifier, name))

  def resolve(self, root):
    return root.resolve(root)

  def evaluate(self, root):
    if 0 and __debug__:
      print
      print root
      print self.resolve(root)
    if not root:
      raise RqlSyntaxError("Unknown syntax node")
    return root.evaluate(self)

if __name__ == '__main__':
  ctx = RqlContext()
  ctx.variables['table.field'] = 20

  print ctx.evaluate(RqlExprParser.parse("1"))
  print ctx.evaluate(RqlExprParser.parse("1 + 2"))
  print ctx.evaluate(RqlExprParser.parse("FALSE"))
  print ctx.evaluate(RqlExprParser.parse("'foo'"))
  print ctx.evaluate(RqlExprParser.parse("'foo' + \"-\" + 'bar'"))
  print ctx.evaluate(RqlExprParser.parse("NOT 1 + NOT 0"))
  print ctx.evaluate(RqlExprParser.parse("(NOT 1) + (NOT 0)"))
  print ctx.evaluate(RqlExprParser.parse("1 + 2 * 3"))
  print ctx.evaluate(RqlExprParser.parse("1 + (2 * 3)"))
  print ctx.evaluate(RqlExprParser.parse("1 OR 2 AND 3"))
  print ctx.evaluate(RqlExprParser.parse("1 OR (2 AND 3)"))

  print ctx.evaluate(RqlExprParser.parse("sum(1, 2, 3) + 5 * 2 + min(5 / 10, 2 * -3)"))
  print ctx.evaluate(RqlExprParser.parse("table.field + 1 + PI"))
  print ctx.evaluate(RqlExprParser.parse("(15 + 2) != (- 14 + 3)"))
  print ctx.evaluate(RqlExprParser.parse("(15 + 2) <= (14 + 3)"))
  print ctx.evaluate(RqlExprParser.parse("(10 > 0 AND 20 < 2)"))
  print ctx.evaluate(RqlExprParser.parse("(10 > 0 OR 20 < 2)"))

  print RqlExprParser.parse("1 + 10 BETWEEN 1 AND 20 + 2")
  print RqlExprParser.parse("1 + 10 NOT BETWEEN 1 AND 20 + 2")
  print ctx.evaluate(RqlExprParser.parse("11 * 20 / 3"))
  print ctx.evaluate(RqlExprParser.parse("11 * 21 / 2"))
