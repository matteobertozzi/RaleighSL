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

from sql import SqlQueryParser, SqlStatement, SqlSelectStatement, SqlColumnWild, SqlTableWild
from expr import RqlContext, RqlIdentifier, get_expr_identifiers

from collections import defaultdict, OrderedDict

# =============================================================================
#  Results Table Helpers
# =============================================================================
def table_sort_comparer(columns, sort_keys):
  key_indexes = [(columns.index(k), cmp_dir) for k, cmp_dir in sort_keys]
  key_func = lambda row: [row[k] for k in key_indexes]

  def comparer(left, right):
    for key, cmp_dir in key_indexes:
      result = cmp(left[key], right[key])
      if result:
        return result * cmp_dir
    return 0
  return comparer

class ResultsTable(object):
  def __init__(self, name, columns):
    self.columns = list(columns)
    self.name = name
    self.rows = []

  def count(self):
    return len(self.rows)

  def insert(self, values):
    assert len(values) == len(self.columns)
    if isinstance(values, dict):
      values = [values[k] for k in self.columns]
    self.rows.append(values)

  def sort(self, sort_keys):
    comparer = table_sort_comparer(self.columns, sort_keys)
    self.rows = sorted(self.rows, cmp=comparer)

  def __iter__(self):
    for row in self.rows:
      yield {k: v for k, v in zip(self.columns, row)}

class GroupedTable(object):
  def __init__(self, name, columns):
    self.columns = list(columns)
    self.name = name
    self.groups = []

  def sort(self, sort_keys):
    comparer = table_sort_comparer(self.columns, sort_keys)
    for i, rows in enumerate(self.groups):
      self.groups[i] = sorted(rows, cmp=comparer)

  def insert(self, group_rows):
    rows = []
    for values in group_rows:
      assert len(values) == len(self.columns)
      if isinstance(values, dict):
        values = [values[k] for k in self.columns]
      rows.append(values)
    self.groups.append(rows)

  def __iter__(self):
    for rows in self.groups:
      for row in rows:
        yield {k: v for k, v in zip(self.columns, row)}
      #yield {k: None for k in self.columns}

class JoinTable(ResultsTable):
  def __init__(self, table_a, table_b):
    self.table_a = table_a
    self.table_b = table_b
    name = '%s+%s' % (table_a.name, table_b.name)
    columns_a = ['%s.%s' % (table_a.name, c) for c in table_a.columns]
    columns_b = ['%s.%s' % (table_b.name, c) for c in table_b.columns]
    super(JoinTable, self).__init__(name, columns_a + columns_b)

  def insert(self, values_a, values_b):
    assert isinstance(values_a, dict)
    assert isinstance(values_b, dict)

    values_a = [values_a[k] for k in self.table_a.columns]
    values_b = [values_b[k] for k in self.table_b.columns]
    assert len(values_a + values_b) == len(self.columns)
    return super(JoinTable, self).insert(values_a + values_b)

def group_by(table, key, predicate_func=None):
  groups = {}
  for row in table:
    if predicate_func is None or predicate_func(row):
      groups.setdefault(row[key], []).append(row)

  tgroups = GroupedTable(table.name, table.columns)
  for _, g in sorted(groups.iteritems()):
    tgroups.insert(g)

  return tgroups

# =============================================================================
#  Join Helpers - https://gist.github.com/matteobertozzi/1337662
# =============================================================================
def cross_join(table_a, table_b):
  results = JoinTable(table_a, table_b)
  for b_row in table_b:
    for a_row in table_a:
      results.insert(a_row, b_row)
  return results

def inner_join(table_a, table_b, predicate_func):
  results = JoinTable(table_a, table_b)
  for a_row in table_a:
    for b_row in table_b:
      if predicate_func(a_row, b_row):
        results.insert(a_row, b_row)
  return results

def equi_join(table_a, table_b, key_a, key_b):
  return inner_join(table_a, table_b, lambda a, b: a[key_a] == b[key_b])

def natural_join(table_a, table_b):
  keys = set(table_a.columns) & set(table_b.columns)
  predicate = lambda a, b: all([a[k] == b[k] for k in keys])
  return inner_join(table_a, table_b, predicate)

def left_outer_join(table_a, table_b, predicate_func):
  null_b_row = dict((k, None) for k in table_b.columns)
  results = JoinTable(table_a, table_b)
  for a_row in table_a:
    match = False
    for b_row in table_b:
      if predicate_func(a_row, b_row):
        results.insert(a_row, b_row)
        match = True
    if match == False:
      results.insert(a_row, null_b_row)
  return results

def right_outer_join(table_a, table_b, predicate_func):
  return left_outer_join(table_b, table_a, predicate_func)

# =============================================================================
#  Debug Helpers
# =============================================================================
def show_table(columns, rows):
  def column_rows(column):
    # TODO: get column default value
    for row in rows:
      yield row.get(column)

  if not columns:
    columns = rows[0].keys()

  lencols = [max(len(str(row)) for row in column_rows(k))
                               for k in columns]
  lencols = [max(clen, len(c)) for clen, c in zip(lencols, columns)]

  hfrmt = ' {0:^{w}} '
  rfrmt = ' {0:<{w}} '

  separator = '+%s+' % '+'.join(['-' * (width + 2) for width in lencols])
  print separator
  print '|%s|' % '|'.join([hfrmt.format(c, w=w) for c, w in zip(columns, lencols)])
  print separator
  for row in rows:
    # TODO: get column default value
    row_values = [row.get(c) for c in columns]
    print '|%s|' % '|'.join([rfrmt.format(c, w=w) for c, w in zip(row_values, lencols)])
  print separator

# =============================================================================
#  Data Table Helpers
# =============================================================================
class Table(object):
  def __init__(self):
    self.columns = OrderedDict()
    self.rows = []

  def show(self):
    show_table(self.columns, self.rows)

  def add_column(self, name, data_type, constraint):
    self.columns[name] = (data_type, constraint)

  def drop_column(self, name):
    del self.columns[name]

  def insert(self, items):
    self.rows.append(items)

  def select(self, name, columns, where):
    results = ResultsTable(name, columns)
    for row in self.rows:
      match = True
      if where:
        ctx = RqlContext()
        ctx.variables.update(row)
        match = ctx.evaluate(where.predicate)
      if match:
        results.insert([row[field] for field in columns])
    return results

# =============================================================================
#  Sql Engine Helpers
# =============================================================================
class SqlEngine(object):
  def __init__(self):
    self.tables = {}

  def show_tables(self):
    for table_name, table in self.tables.iteritems():
      print 'TABLE', table_name, table.columns
      table.show()

  def parse(self, sql):
    ctx = RqlContext()
    return [stmt.resolve(ctx) for stmt in SqlQueryParser.parse(sql)]

  def execute(self, sql):
    for stmt in self.parse(sql):
      self.execute_statement(stmt)

  def fetch(self, sql):
    for stmt in self.parse(sql):
      assert isinstance(stmt, SqlSelectStatement)
      yield self.execute_statement(stmt)

  def execute_statement(self, stmt):
    #print stmt
    assert isinstance(stmt, SqlStatement), type(stmt)
    operation = stmt.__class__.__name__[3:-9]
    return getattr(self, operation)(stmt)

  def CreateTable(self, stmt):
    table = Table()
    for column in stmt.column_definitions:
      table.add_column(column.identifier, column.data_type, column.constraint)
    self.tables[stmt.name.name] = table
    return table

  def AlterTable(self, stmt):
    table = self.tables[stmt.name.name]
    if stmt.operation == 'ADD':
      table.add_column(stmt.column.identifier, stmt.column.data_type, stmt.column.constraint)
    elif stmt.operation == 'DROP':
      table.drop_column(stmt.column)

  def DropTable(self, stmt):
    del self.tables[stmt.name.name]

  def Insert(self, stmt):
    table = self.tables[stmt.destination.name]
    ctx = RqlContext()
    table.insert({name: ctx.evaluate(expr) for name, expr in zip(stmt.columns, stmt.values)})

  def Update(self, stmt):
    table = self.tables[stmt.source.name]

  def Select(self, stmt):
    alias_map = {}
    alias_rmap = {}

    # Extract the tables to scan
    source_tables = set()
    tables = {}
    for source in stmt.get_data_sources():
      if source.query:
        name = source.alias or str(source.query)
        tables[name] = Select(source.query)
      else:
        name = source.name.name
        source_tables.add(name)
        tables[name] = self.tables[name]
        if source.alias:
          alias_map[source.alias] = name
          alias_rmap[name] = source.aliases

    # Map where-clauses result-fields to tables
    expr_fields = []
    table_results_map = defaultdict(list)
    for index, result in enumerate(stmt.results):
      if isinstance(result, SqlColumnWild):
        for source in source_tables:
          table_results_map[source].extend(alias_map.get(k, k) for k in self.tables[source].columns)
      elif isinstance(result, SqlTableWild):
        table_results_map[result.table].extend(alias_map.get(k, k) for k in self.tables[source].columns)
      elif isinstance(result.expr, RqlIdentifier):
        field_name = result.expr.name
        index = field_name.rfind('.')
        if index > 0:
          source = field_name[:index]
          field_name = field_name[1+index:]
          table_results_map[source].append(field_name)
        else:
          for source in source_tables:
            table = self.tables[source]
            if field_name in table.columns:
              table_results_map[source].append(field_name)
              break
        # TODO: Extract where clause for the field
      else:
        # TODO: Expr may contains references to a table
        expr_fields.append((index, result))

      if result.alias:
        name = str(result.expr)
        alias_map[result.alias] = name
        alias_rmap[name] = result.alias

    # Scan each source
    results = []
    for source, fields in table_results_map.iteritems():
      result = self.tables[source].select(source, fields, stmt.where)
      results.append(result)

    # Join and apply where-clauses
    if len(results) > 1:
      results = list(reversed(results))
      # TODO: Apply the proper join...
      while len(results) >= 2:
        res_a = results.pop()
        res_b = results.pop()
        join_result = cross_join(res_a, res_b)
        results.append(join_result)
    results = results[0]

    if expr_fields:
      for row in results.rows:
        new_values = []
        for index, field in expr_fields:
          ctx = RqlContext()
          ctx.variables.update(dict(zip(results.columns, row)))
          ctx.variables.update({alias_map.get(k, k): v for k, v in zip(results.columns, row)})
          value = ctx.evaluate(field.expr)
          new_values.append((index, value))
        for index, value in new_values:
          row.insert(index, value)

      for index, field in expr_fields:
        results.columns.insert(index, str(field.expr))

    # Replace column names with the alias
    # blah.. move up to the result_table.aliases
    results.columns = [alias_map.get(name, name) for name in results.columns]

    if stmt.group_by:
      key = stmt.group_by.groups[0].name
      results = group_by(results, key)

    if stmt.order_by:
      results.sort([(o.column, -1 if o.order_type == 'DESC' else 1) for o in stmt.order_by])

    return results

def test_crud():
  engine = SqlEngine()
  engine.execute('CREATE TABLE foo (key INT, value INT);')

  for i in xrange(10):
    engine.execute('INSERT INTO foo (key, value) VALUES (%d, %d);' % (i, i * 100))
  engine.show_tables()

  engine.execute('ALTER TABLE foo ADD a INT;')
  engine.show_tables()

  for i in xrange(10):
    engine.execute('UPDATE foo SET a = %d, value = %d WHERE key = %d;' % (i * 10, i * 1000, i))
  engine.show_tables()

  for result in engine.fetch('SELECT (1 + key), value, key FROM foo WHERE key > 4 AND value <= 700;'):
    show_table(None, list(result))

  engine.execute('ALTER TABLE foo DROP value;')
  engine.show_tables()

  engine.execute('DROP TABLE foo;')
  engine.show_tables()

if __name__ == '__main__':
  engine = SqlEngine()
  engine.execute('CREATE TABLE foo (key INT, value INT);')
  engine.execute('CREATE TABLE foo2 (key INT, value INT);')

  for i in xrange(5):
    engine.execute('INSERT INTO foo (key, value) VALUES (%d, %d);' % (i, i * 100))
    engine.execute('INSERT INTO foo2 (key, value) VALUES (%d, %d);' % (i, i * 200))

  for result in engine.fetch('SELECT * FROM foo;'):
    show_table(None, list(result))

  for result in engine.fetch('SELECT * FROM foo, foo2;'):
    show_table(None, list(result))

  for result in engine.fetch('SELECT (1 + key), value, key FROM foo, foo2 WHERE key > 4 AND value <= 700;'):
    show_table(None, list(result))

  for result in engine.fetch('SELECT (foo.value + foo2.value) AS foo_sum, foo.key, (foo.key + 1), foo.value, foo2.value FROM foo INNER JOIN foo2 ON foo.key = foo2.key WHERE key > 4 AND key <= 8 GROUP by foo.value ORDER BY foo2.value DESC;'):
    show_table(result.columns, result)
