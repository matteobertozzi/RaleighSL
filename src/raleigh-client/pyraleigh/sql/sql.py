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
from expr import RqlParser, RqlExprParser, RqlAssignment, RqlString

class SqlFragment(object):
  def __repr__(self):
    return '%s(%r)' % (self.__class__.__name__, self.__dict__)

  def resolve(self, context):
    for key, value in self.__dict__.items():
      resolve_func = getattr(value, 'resolve', None)
      if resolve_func:
        value = resolve_func(context)
        setattr(self, key, value)
    return self

class SqlStatement(SqlFragment):
  pass

class SqlDataType(SqlFragment):
  def __init__(self, name):
    self.name = name

class SqlSizedDataType(SqlDataType):
  def __init__(self, name, size):
    super(SqlSizedDataType, self).__init__(name)
    self.size = size

class SqlDecimalDataType(SqlSizedDataType):
  def __init__(self, name, size, decimals):
    super(SqlDecimalDataType, self).__init__(name, size)
    self.decimals = decimals

# =============================================================================
#  Constraints
# =============================================================================
class SqlConstraint(SqlFragment):
  pass

class SqlCheckConstraint(SqlConstraint):
  def __init__(self, predicate):
    self.predicate = predicate

class SqlDefaultConstraint(SqlConstraint):
  def __init__(self, expression):
    self.expression = expression

class SqlForeignKeyConstraint(SqlConstraint):
  pass

class SqlNullableConstraint(SqlConstraint):
  def __init__(self, nullable):
    self.nullable = nullable

class SqlUniqueConstraint(SqlConstraint):
  def __init__(self, conflict_clause):
    self.conflict_clause = conflict_clause

class SqlPrimaryKeyConstraint(SqlConstraint):
  pass

class SqlConflictClause(SqlFragment):
  __slots__ = ('operation')

  def __init__(self, operation):
    self.operation = operation

class SqlJoin(SqlFragment):
  pass

class SqlJoinOp(SqlFragment):
  pass

class SqlJoinSource(SqlFragment):
  pass

class SqlDataSource(SqlFragment):
  def __init__(self):
    self.query = None
    self.alias = None
    self.name = None

# =============================================================================
#  Column
# =============================================================================
class SqlWhereClause(SqlFragment):
  def __init__(self):
    self.predicate = None

class SqlGroupByClause(SqlFragment):
  def __init__(self):
    self.groups = None

class SqlHavingClause(SqlFragment):
  def __init__(self):
    self.expression = None

class SqlColumnWithSortOrder(SqlFragment):
  pass

class SqlColumnDefinition(SqlFragment):
  def __init__(self, identifier, data_type, constraint):
    self.data_type = data_type
    self.identifier = identifier
    self.constraint = constraint

class SqlCountFunction(SqlFragment):
  pass

class SqlExprParser(RqlExprParser):
  def parse_primary(self):
    if self.lexer.peek_seq('SELECT'):
      return SqlSelectParser(self.lexer).parse_select()

    if self.lexer.match_seq('COUNT'):
      self.lexer.expect_seq('(')
      count = SqlCountFunction()
      count.filter = self.lexer.match_one('DISTINCT')
      count.field = self.lexer.expect_string_or_identifier()
      self.lexer.expect_seq(')')
      return count

    token = self.lexer.peek()
    if token.is_string():
      token = self.lexer.next()
      if self.lexer.match_seq('AS'):
        string_as_type = self.lexer.expect_one('DATE')
      return RqlString(token.value)

    return super(SqlExprParser, self).parse_primary()

  def parse_predicate(self):
    self.lexer.set_replacements({'=': '=='})
    try:
      return super(SqlExprParser, self).parse_predicate()
    finally:
      self.lexer.set_replacements(dict())

class SqlParser(RqlParser):
  # ===========================================================================
  #  Expressions
  # ===========================================================================
  def parse_assignment(self):
    assignment = SqlExprParser(self.lexer).parse_assignment()
    if not isinstance(assignment, RqlAssignment):
      raise RqlSyntaxError("Expected Assignment, got %r" % assignment)
    return assignment

  def parse_predicate(self):
    return SqlExprParser(self.lexer).parse_predicate()

  def parse_expression(self):
    return SqlExprParser(self.lexer).parse_expression()

  # ===========================================================================
  #  Data Type
  # ===========================================================================
  def parse_data_type(self):
    type_name = self.lexer.expect_string_or_identifier()
    if self.lexer.match_seq('('):
      size = self.lexer.expect_numeric()
      if self.lexer.match_seq(','):
        decimals = self.lexer.expect_numeric()
        self.lexer.expect_seq(')')
        return SqlDecimalDataType(type_name, size, decimals)
      self.lexer.expect_seq(')')
      return SqlSizedDataType(type_name, size)
    return SqlDataType(type_name)

  def parse_data_type_constraint(self):
    if self.lexer.peek_seq('PRIMARY', 'KEY'):
      primary_key = SqlPrimaryKeyConstraint()
      primary_key.order_type = self.lexer.match_one('ASC', 'DESC')
      primary_key.conflict_clause = self.parse_conflict_clause()
      primary_key.autoincrement = self.match_one('AUTOINCREMENT')
      return primary_key
    if self.lexer.match_seq('NOT', 'NULL'):
      return SqlNullableConstraint(False)
    if self.lexer.match_seq('UNIQUE'):
      conflict_clause = self.parse_conflict_clause()
      return SqlUniqueConstraint(conflict_clause)
    if self.lexer.match_seq('CHECK'):
      predicate = self.parse_predicate()
      return SqlCheckConstraint(predicate)
    if self.lexer.match_seq('DEFAULT'):
      expr = self.parse_expression()
      return SqlDefaultConstraint(expr)
    if self.lexer.peek_one('REFERENCES'):
      return self.parse_foreign_key_clause()
    return None

  # ===========================================================================
  #  Conflicts
  # ===========================================================================
  def parse_conflict_clause(self):
    if self.match_seq('ON', 'CONFLICT'):
      return SqlConflictClause(self.parse_conflict_resolution())
    return None

  def parse_conflict_resolution(self):
    return self.lexer.match_one('ROLLBACK', 'ABORT', 'REPLACE', 'FAIL', 'IGNORE')

  # ===========================================================================
  #  Table Column
  # ===========================================================================
  def parse_column_definition(self):
    name = self.parse_column_name()
    data_type = self.parse_data_type()
    constraint = self.parse_data_type_constraint()
    return SqlColumnDefinition(name, data_type, constraint)

  def parse_column_name(self):
    return self.lexer.expect_string_or_identifier().lower()

  # ===========================================================================
  #  Helpers
  # ===========================================================================
  def try_parse_where(self):
    if self.lexer.match_seq('WHERE'):
      frag = SqlWhereClause()
      frag.predicate = self.parse_predicate()
      return frag
    return None

  def try_parse_alias(self):
    if self.lexer.match_seq('AS'):
      alias = self.lexer.expect_string_or_identifier()
      return alias
    return None

  def parse_foreign_key_clause(self):
    self.lexer.expect_seq('REFERENCES')

    constraint = SqlForeignKeyConstraint()
    constraint.table = self.parse_identifier()

    if self.lexer.peek('('):
      constraint.columns = self.parse_definition_group(self.parse_column_name)

    if self.lexer.match_seq('ON'):
      self.lexer.match_one('DELETE', 'UPDATE')
      if self.lexer.match_seq('SET', 'NULL'):
        pass
      elif self.lexer.match_seq('SET', 'DEFAULT'):
        pass
      elif self.lexer.match_seq('CASCADE'):
        pass
      if self.lexer.match_seq('RESTRICT'):
        pass
      if self.lexer.match_seq('NO', 'ACTION'):
        pass

    if self.lexer.match_one(('NOT', 'DEFERRABLE'), 'DEFERRABLE'):
      if self.match_seq('INITIALLY', 'DEFERRED'):
        pass
      elif self.match_seq('INITIALLY', 'IMMEDIATE'):
        pass

    return constraint

  def parse_definition_group(self, *item_parse_funcs):
    self.lexer.expect_seq('(')

    def_group = [None] * len(item_parse_funcs)
    for i, item_parse_func in enumerate(item_parse_funcs):
      if self.lexer.peek_one(')'):
        break

      def_list = self.parse_comma_list(item_parse_func)
      def_group[i] = def_list
    self.lexer.expect_seq(')')

    return def_group if len(item_parse_funcs) > 1 else def_group[0]

# =============================================================================
#  Select
# =============================================================================
class SqlSelectStatement(SqlStatement):
  COMMAND = 'SELECT'

  def __init__(self):
    self.where = None
    self.group_by = None
    self.order_by = None

  def get_data_sources(self):
    for source in self.sources:
      if isinstance(source, SqlJoin):
        source = source.source
      assert isinstance(source, SqlDataSource)
      yield source
    yield source

class SqlColumnExpr(SqlFragment):
  def __init__(self, expr, alias):
    self.alias = alias
    self.expr = expr

class SqlColumnWild(SqlFragment):
  def __init__(self):
    self.alias = None

class SqlTableWild(SqlFragment):
  def __init__(self, table):
    self.table = table
    self.alias = None

class SqlSelectParser(SqlParser):
  def parse_select(self):
    stmt = self.parse_select_core()
    #self.parse_compound_operator()
    if self.lexer.match_seq('ORDER', 'BY'):
      stmt.order_by = self.parse_comma_list(self.parse_oredering_term)
    if self.lexer.match_seq('LIMIT'):
      stmt.limit_expr = self.parse_expression()
      if self.lexer.match_one('OFFSET', ','):
        stmt.offset_expr = self.parse_expression()
    return stmt

  def parse_compound_operator(self):
    operator = self.lexer.match_one(('UNION', 'ALL'), 'UNION', 'INTERSECT', 'EXCEPT')

  def parse_oredering_term(self):
    stmt = SqlColumnWithSortOrder()
    stmt.column = self.parse_column_name()
    stmt.order_type = self.lexer.match_one('ASC', 'DESC')
    return stmt

  def parse_select_core(self):
    self.lexer.expect_seq('SELECT')

    stmt = SqlSelectStatement()
    stmt.restriction = self.lexer.match_one('DISTINCT', 'ALL')
    if self.lexer.match_one('TOP'):
      stmt.top = self.lexer.expect_numeric()
    stmt.results = self.parse_comma_list(self.parse_result_column)

    if self.lexer.peek_one('FROM'):
      stmt.sources = self.parse_from()
    stmt.where = self.try_parse_where()
    if self.lexer.peek_seq('GROUP', 'BY'):
      stmt.group_by = self.parse_group_by()
      if self.lexer.peek_one('HAVING'):
        stmt.having = self.parse_having()
    return stmt

  def parse_result_column(self):
    if self.lexer.match_seq('*'):
      return SqlColumnWild()
    if self.lexer.peek_seq(None, '.', '*'):
      table_name = self.lexer.expect_string_or_identifier()
      self.lexer.expect_seq('.', '*')
      return SqlTableWild(table_name)

    expr = self.parse_expression()
    alias = self.try_parse_alias()
    return SqlColumnExpr(expr, alias)

  def parse_from(self):
    self.lexer.expect_seq('FROM')

    sources = [self.parse_single_source()]
    while True:
      join_op = self.parse_join_op()
      if not join_op: break

      join = SqlJoin()
      join.operation = join_op
      join.source = self.parse_single_source()
      join.constraint = self.parse_join_constraint()
      sources.append(join)
    return sources

  def parse_join_op(self):
    frag = SqlJoinOp()
    if self.lexer.match_seq(','):
      return frag

    frag.natural = self.lexer.match_one('NATURAL')
    frag.join_type = self.lexer.match_one(('LEFT', 'OUTER'), 'LEFT', 'INNER', 'CROSS')
    if not frag.natural and not frag.join_type and not self.lexer.match_seq('JOIN'):
      return None
    self.lexer.expect_seq('JOIN')
    return frag

  def parse_join_constraint(self):
    if self.lexer.match_seq('ON'):
      return self.parse_predicate()
    if self.lexer.match_seq('USING'):
      return 'USING', self.parse_comma_list(self.parse_column_name)
    return None

  def parse_single_source(self):
    frag = SqlDataSource()
    if self.lexer.match_seq('('):
      if self.lexer.peek_seq('SELECT'):
        frag.query = self.parse_select()
        self.lexer.expect_seq(')')
        frag.alias = self.try_parse_alias()
      else:
        return self.parse_single_source()
    else:
      frag.name = self.parse_identifier()
      frag.alias = self.try_parse_alias()

      if self.lexer.match_seq('INDEXED', 'BY'):
        frag.index_name = self.lexer.next()
      elif self.lexer.match_seq('NOT', 'INDEXED'):
        pass
    return frag

  def parse_group_by(self):
    self.lexer.expect_seq('GROUP', 'BY')

    frag = SqlGroupByClause()
    frag.groups = self.parse_comma_list(self.parse_expression)
    return frag

  def parse_having(self):
    self.lexer.expect_seq('HAVING')

    frag = SqlHavingClause()
    frag.expression = self.parse_predicate()
    return frag

# =============================================================================
#  Insert
# =============================================================================
class SqlInsertStatement(SqlStatement):
  COMMAND = 'INSERT'

  def __init__(self):
    self.conflict_resolution = None
    self.destination = None
    self.columns = None
    self.values = None
    self.query = None

class SqlInsertParser(SqlParser):
  def parse_insert(self):
    self.lexer.expect_seq('INSERT')

    stmt = SqlInsertStatement()
    if self.lexer.match_seq('OR'):
      stmt.conflict_resolution = self.parse_conflict_resolution()

    self.lexer.expect_seq('INTO')
    stmt.destination = self.parse_identifier()

    if self.lexer.match_seq('DEFAULT', 'VALUES'):
      return stmt

    if self.lexer.peek_seq('('):
      stmt.columns = self.parse_definition_group(self.parse_column_name)

    if self.lexer.match_seq('VALUES'):
      stmt.values = self.parse_definition_group(self.parse_expression)
    elif self.lexer.peek_seq('SELECT'):
      stmt.query = SqlSelectParser(self.lexer).parse_select()

    return stmt

# =============================================================================
#  Update
# =============================================================================
class SqlUpdateStatement(SqlStatement):
  COMMAND = 'UPDATE'

  def __init__(self):
    self.conflict_resolution = None
    self.source = None
    self.fields = None
    self.where = None

class SqlUpdateParser(SqlParser):
  def parse_update(self):
    self.lexer.expect_seq('UPDATE')

    stmt = SqlUpdateStatement()
    if self.lexer.match_seq('OR'):
      stmt.conflict_resolution = self.parse_conflict_resolution()

    stmt.source = self.parse_identifier()

    self.lexer.expect_seq('SET')
    stmt.fields = self.parse_comma_list(self.parse_assignment)

    stmt.where = self.try_parse_where()
    return stmt

# =============================================================================
#  Delete
# =============================================================================
class SqlDeleteStatement(SqlStatement):
  COMMAND = 'DELETE'

  def __init__(self):
    self.name = None
    self.index_name = None
    self.where = None

class SqlDeleteParser(SqlParser):
  def parse_delete(self):
    self.lexer.expect_seq('DELETE', 'FROM')

    stmt = SqlDeleteStatement()
    stmt.name = self.parse_identifier()

    if self.lexer.match_seq('INDEXED', 'BY'):
      stmt.index_name = self.lexer.next()
    elif self.lexer.match_seq('NOT', 'INDEXED'):
      pass

    stmt.where = self.try_parse_where()
    return stmt

# =============================================================================
#  Create
# =============================================================================
class SqlCreateTableStatement(SqlStatement):
  COMMAND = 'CREATE'

  def __init__(self):
    self.if_not_exists = False
    self.temporary = False
    self.name = None
    self.column_definitions = []
    self.table_constraints = []

class SqlCreateParser(SqlParser):
  # CREATE [TEMP|TEMPORARY] TABLE [IF NOT EXISTS] name
  def parse_create(self):
    self.lexer.expect_seq('CREATE')

    temporary = self.lexer.match_one('TEMP', 'TEMPORARY')
    object_type = self.lexer.expect_identifier()
    if_not_exists = self.lexer.match_seq('IF', 'NOT', 'EXISTS')
    object_identifier = self.parse_identifier()

    stmt = self._call_method('parse_%s' % object_type.lower())
    stmt.if_not_exists = if_not_exists
    stmt.name = object_identifier
    stmt.temporary = temporary
    return stmt

  def parse_table(self):
    stmt = SqlCreateTableStatement()
    if self.lexer.peek_seq('('):
      columns, constraints = self.parse_definition_group(self.parse_column_definition, self.parse_table_constraint)
      stmt.column_definitions = columns
      stmt.table_constraints = constraints
    return stmt

  def parse_table_constraint(self):
    if self.lexer.match_seq('CONSTRAINT'):
      name = self.lexer.expect_string_or_identifier()

    if self.lexer.peek_seq('PRIMARY', 'KEY'):
      columns = self.parse_definition_group(self.parse_column_name)
      conflict_clause = self.parse_conflict_clause()
      return SqlUniqueConstraint(conflict_clause) # TODO: PRIMARY KEY
    if self.lexer.match_seq('UNIQUE'):
      columns = self.parse_definition_group(self.parse_column_name)
      conflict_clause = self.parse_conflict_clause()
      return SqlUniqueConstraint(conflict_clause)
    if self.lexer.match_seq('CHECK'):
      predicate = self.parse_predicate()
      return SqlCheckConstraint(predicate)
    if self.lexer.match_seq('FOREIGN', 'KEY'):
      columns = self.parse_definition_group(self.parse_column_name)
      return self.parse_foreign_key_clause()

    return None

# =============================================================================
#  Alter
# =============================================================================
class SqlAlterTableStatement(SqlStatement):
  COMMAND = 'ALTER'

  def __init__(self):
    self.name = None
    self.type = None
    self.operation = None
    self.column = None

class SqlAlterParser(SqlParser):
  def parse_alter(self):
    self.lexer.expect_seq('ALTER')

    object_type = self.lexer.expect_string_or_identifier()
    object_identifier = self.parse_identifier()

    stmt = self._call_method('parse_%s' % object_type.lower())
    stmt.name = object_identifier
    return stmt

  def parse_table(self):
    stmt = SqlAlterTableStatement()
    if self.lexer.match_seq('ADD'):
      stmt.operation = 'ADD'
      stmt.type = self.lexer.match_one('COLUMN')
      stmt.column = self.parse_column_definition()
    elif self.lexer.match_seq('DROP'):
      stmt.operation = 'DROP'
      stmt.type = self.lexer.match_one('COLUMN')
      stmt.column = self.parse_column_name()
    else:
      raise RqlSyntaxError("Unexpected alter operation %s" % self.lexer.peek())
    return stmt

# =============================================================================
#  Drop
# =============================================================================
class SqlDropTableStatement(SqlStatement):
  COMMAND = 'DROP'

  def __init__(self):
    self.if_exists = False
    self.name = None

class SqlDropParser(SqlParser):
  def parse_drop(self):
    self.lexer.expect_seq('DROP')

    object_type = self.lexer.expect_string_or_identifier()

    if_exists = self.lexer.match_seq('IF', 'EXISTS')

    object_identifier = self.parse_identifier()

    stmt = self._call_method('parse_%s' % object_type.lower())
    stmt.name = object_identifier
    stmt.if_exists = if_exists
    return stmt

  def parse_table(self):
    return SqlDropTableStatement()

# =============================================================================
#  Begin/Commit/Rollback
# =============================================================================
class SqlBeginStatement(SqlStatement):
  COMMAND = 'BEGIN'

  def __init__(self):
    self.type = None
    self.mode = None

class SqlBeginParser(SqlParser):
  def parse_begin(self):
    self.lexer.expect_seq('BEGIN')

    stmt = SqlBeginStatement()
    stmt.mode = self.lexer.match_one('DEFERRED', 'IMMEDIATE', 'EXCLUSIVE')
    stmt.type = self.lexer.match_one('TRANSACTION')
    return stmt

class SqlCommitStatement(SqlStatement):
  COMMAND = 'COMMIT'

  def __init__(self):
    self.type = None

class SqlCommitParser(SqlParser):
  def parse_commit(self):
    self.lexer.expect_one('COMMIT', 'END')

    stmt = SqlCommitStatement()
    stmt.type = self.lexer.match_one('TRANSACTION')
    return stmt

class SqlRollbackStatement(SqlStatement):
  COMMAND = 'ROLLBACK'

  def __init__(self):
    self.type = None
    self.savepoint = None

class SqlRollbackParser(SqlParser):
  def parse_rollback(self):
    self.lexer.expect_seq('ROLLBACK')

    stmt = SqlRollbackStatement()
    stmt.type = self.lexer.match_one('TRANSACTION')

    if self.lexer.match_one(('TO', 'SAVEPOINT'), 'TO'):
      stmt.savepoint = self.lexer.expect_string_or_identifier()

    return stmt

# =============================================================================
#  SavePoint/Release
# =============================================================================
class SqlSavePointStatement(SqlStatement):
  COMMAND = 'SAVEPOINT'

  def __init__(self):
    self.name = None

class SqlSavePointParser(SqlParser):
  def parse_save_point(self):
    self.lexer.expect_seq('SAVEPOINT')

    stmt = SqlSavePointStatement()
    stmt.name = self.lexer.expect_string_or_identifier()
    return stmt

class SqlReleaseStatement(SqlStatement):
  COMMAND = 'RELEASE'

  def __init__(self):
    self.type = None
    self.name = None

class SqlReleaseParser(SqlParser):
  def parse_release(self):
    self.lexer.expect_seq('RELEASE')

    stmt = SqlReleaseStatement()
    stmt.type = self.lexer.match_one('SAVEPOINT')
    stmt.name = self.lexer.expect_string_or_identifier()
    return stmt

# =============================================================================
#  Explain
# =============================================================================
class SqlExplainStatement(SqlStatement):
  COMMAND = 'EXPLAIN'

  def __init__(self):
    self.query_plan = False
    self.query = None

class SqlExplainParser(SqlParser):
  def parse_explain(self):
    self.lexer.expect_seq('EXPLAIN')

    stmt = SqlExplainStatement()
    stmt.query_plan = self.lexer.match_seq('QUERY', 'PLAN')
    stmt.query = SqlQueryParser(self.lexer).parse_statement()
    return stmt

# =============================================================================
#  Analyze
# =============================================================================
class SqlAnalyzeStatement(SqlStatement):
  COMMAND = 'ANALYZE'

  def __init__(self):
    self.object = None

class SqlAnalyzeParser(SqlParser):
  def parse_analyze(self):
    self.lexer.expect_seq('ANALYZE')

    stmt = SqlAnalyzeStatement()
    token = self.lexer.peek()
    if token and (token.is_identifier() or token.is_string()):
      stmt.object = self.parse_identifier()
    return stmt

# =============================================================================
#  Parser
# =============================================================================
class SqlQueryParser(SqlParser):
  def __init__(self, lexer):
    self.lexer = lexer

  def parse_statement(self):
    # DML
    if self.lexer.peek_one('SELECT'):
      return SqlSelectParser(self.lexer).parse_select()
    if self.lexer.peek_one('INSERT'):
      return SqlInsertParser(self.lexer).parse_insert()
    if self.lexer.peek_one('UPDATE'):
      return SqlUpdateParser(self.lexer).parse_update()
    if self.lexer.peek_one('DELETE'):
      return SqlDeleteParser(self.lexer).parse_delete()
    if self.lexer.peek_one('MERGE'):
      return SqlMergeParser(self.lexer).parse_merge()

    # DDL
    if self.lexer.peek_one('CREATE'):
      return SqlCreateParser(self.lexer).parse_create()
    if self.lexer.peek_one('ALTER'):
      return SqlAlterParser(self.lexer).parse_alter()
    if self.lexer.peek_one('DROP'):
      return SqlDropParser(self.lexer).parse_drop()

    # PRG
    if self.lexer.peek_one('BEGIN'):
      return SqlBeginParser(self.lexer).parse_begin()
    if self.lexer.peek_one('COMMIT', 'END'):
      return SqlCommitParser(self.lexer).parse_commit()
    if self.lexer.peek_one('ROLLBACK'):
      return SqlRollbackParser(self.lexer).parse_rollback()
    if self.lexer.peek_one('SAVEPOINT'):
      return SqlSavePointParser(self.lexer).parse_save_point()
    if self.lexer.peek_one('RELEASE'):
      return SqlReleaseParser(self.lexer).parse_release()

    if self.lexer.peek_one('EXPLAIN'):
      return SqlExplainParser(self.lexer).parse_explain()
    if self.lexer.peek_one('ANALYZE'):
      return SqlAnalyzeParser(self.lexer).parse_analyze()

    if self.lexer.has_next():
      raise RqlSyntaxError("Unexpected token %r" % self.lexer.peek())
    return None

  # <statementList> ::= <Statement>
  #                   | <Statement> <StatementList>
  def parse_statement_list(self):
    statements = []
    while self.lexer.has_next():
      stmt = self.parse_statement()
      if not stmt:
        raise RqlSyntaxError("Unexpected null statement result")
      statements.append(stmt)
      self.lexer.expect_seq(';')
    return statements

  @staticmethod
  def parse(query):
    lexer = RqlLexer(query)
    return SqlQueryParser(lexer).parse_statement_list()

  @staticmethod
  def parse_file(path):
    fd = open(path)
    try:
      #blob = remove_comments(fd.read())
      blob = fd.read()
    finally:
      fd.close()

    lexer = RqlLexer(blob)
    return SqlQueryParser(lexer).parse_statement_list()

if __name__ == '__main__':
  query = """
CREATE TABLE foo (id INT, data VARCHAR(64));

ALTER TABLE table_name ADD column_name INT NOT NULL;
ALTER TABLE table_name DROP COLUMN column_name;

DROP TABLE foo;
DROP TABLE IF EXISTS foo;

BEGIN;
BEGIN DEFERRED;
BEGIN IMMEDIATE;
BEGIN EXCLUSIVE;
BEGIN TRANSACTION;
BEGIN EXCLUSIVE TRANSACTION;

END;
END TRANSACTION;

COMMIT;
COMMIT TRANSACTION;

ROLLBACK;
ROLLBACK TRANSACTION;
ROLLBACK TRANSACTION TO mysavepoint;
ROLLBACK TRANSACTION TO SAVEPOINT mysavepoint;

SAVEPOINT mysavepoint;
RELEASE mysavepoint;
RELEASE SAVEPOINT mysavepoint;

INSERT INTO foo DEFAULT VALUES;
INSERT OR ROLLBACK INTO foo DEFAULT VALUES;
INSERT OR ABORT INTO foo DEFAULT VALUES;
INSERT OR REPLACE INTO foo DEFAULT VALUES;
INSERT OR FAIL INTO foo DEFAULT VALUES;
INSERT OR IGNORE INTO foo DEFAULT VALUES;
INSERT INTO foo VALUES (1 + 5, 2, (5 * 3) + 4);
INSERT INTO foo (a, b, c) VALUES (1, 2, 3);
INSERT INTO foo SELECT * FROM bar;

UPDATE foo SET a = 10;
UPDATE foo SET a = 10, b = (20 * 5);
UPDATE foo SET a = 10, b = 2 WHERE a > 20;
UPDATE OR ROLLBACK foo SET a = 10;
UPDATE OR ABORT foo SET a = 10;
UPDATE OR REPLACE foo SET a = 10;
UPDATE OR FAIL foo SET a = 10;
UPDATE OR IGNORE foo SET a = 10;

DELETE FROM foo;
DELETE FROM foo WHERE a > 20;
DELETE FROM foo INDEXED BY a;
DELETE FROM foo NOT INDEXED WHERE a < 10;

EXPLAIN SELECT * FROM foo;
EXPLAIN QUERY PLAN SELECT * FROM foo;

ANALYZE;
ANALYZE table;
ANALYZE namespace.table;

SELECT a, (10 + 5) FROM tb0 WHERE a > 0;
SELECT (10 * 2) AS a;
SELECT a, b FROM foo ORDER BY a;
SELECT a, b FROM foo ORDER BY a LIMIT (1 + 5);
SELECT ALL a, b FROM foo ORDER BY a ASC LIMIT (1 + 5), (5 * 4);
SELECT DISTINCT a, b FROM foo ORDER BY a DESC LIMIT (1 + 5) OFFSET (5 * 4);
SELECT a, b FROM tb0 WHERE a > 0 AND b < 2;
SELECT a, b, c FROM tb0 GROUP BY a, b;
SELECT a, b, c FROM tb0 GROUP BY a, b HAVING a > 5 AND b <= 3;

SELECT Orders.OrderID, Customers.CustomerName, Orders.OrderDate FROM Orders INNER JOIN Customers ON Orders.CustomerID=Customers.CustomerID;
"""
  for stmt in SqlQueryParser.parse(query):
    print stmt

  from time import time
  st = time()
  count = 0
  for i in xrange(1000):
    count += len(SqlQueryParser.parse(query))
  et = time()
  print '%dquery %.3fsec (%.2fquery/sec)' % (count, (et - st), count / (et - st))
