# th30z@u1310:[Desktop]$ psql -h localhost -p 55432
# Password:
# psql (9.1.10, server 0.0.0)
# WARNING: psql version 9.1, server version 0.0.
#          Some psql features might not work.
# Type "help" for help.
#
# th30z=> select foo;
#  a | b
# ---+---
#  1 | 2
#  3 | 4
#  5 | 6
# (3 rows)

from cStringIO import StringIO
from time import time

import SocketServer
import logging
import struct

from engine import SqlEngine
from sql import SqlQueryParser, SqlStatement, SqlSelectStatement

PSQL_FE_MESSAGES = {
  'p': "Password message",
  'Q': "Simple query",
  'P': "Parse",
  'B': "Bind",
  'E': "Execute",
  'D': "Describe",
  'C': "Close",
  'H': "Flush",
  'S': "Sync",
  'F': "Function call",
  'd': "Copy data",
  'c': "Copy completion",
  'f': "Copy failure",
  'X': "Termination",
}

class IntField:
  def __init__(self, name):
    self.name = name
    self.type_id = 23
    self.type_size = 4

class PgBuffer(object):
  def __init__(self, stream):
    self.buffer = stream

  def read_byte(self):
    return self.read_bytes(1)

  def read_bytes(self, n):
    data = self.buffer.read(n)
    if not data:
      raise Exception("No data")
    return data

  def read_string(self, n):
    data = self.read_bytes(n)
    assert data[-1] == '\x00', repr(data)
    return data[:-1]

  def read_int32(self):
    data = self.read_bytes(4)
    return struct.unpack("!i", data)[0]

  def read_parameters(self, n):
    data = list(reversed(self.read_bytes(n).split('\x00')))
    parameters = {}
    while len(data) >= 2:
      key = data.pop()
      value = data.pop()
      if key and value:
        parameters[key] = value
    return parameters

  def write_byte(self, value):
    self.buffer.write(value)

  def write_bytes(self, value):
    self.buffer.write(value)

  def write_int16(self, value):
    self.buffer.write(struct.pack("!h", value))

  def write_int32(self, value):
    self.buffer.write(struct.pack("!i", value))

  def write_string(self, value):
    self.buffer.write(value)
    self.buffer.write('\x00')

  def write_parameters(self, kvs):
    data = ''.join(['%s\x00%s\x00' % kv in kvs])
    self.buffer.write_int32(4 + len(data))
    self.buffer.write(data)

class PsqlHandler(SocketServer.StreamRequestHandler):
  def handle(self):
    self.debug('connected')
    self.engine = SqlEngine()
    self._pgbuf = PgBuffer(self.rfile)
    self.wbuf = PgBuffer(self.wfile)

    try:
      # Handshake
      self.read_ssl_request()
      self.send_notice()
      self.read_startup_message()

      # Auth
      self.send_authentication_request()
      self.read_authentication()
      self.send_authentication_ok()

      while True:
        self.send_ready_for_query()

        # Read Message
        type_code = self._pgbuf.read_byte()
        print PSQL_FE_MESSAGES.get(type_code)

        if type_code == 'Q':
          msglen = self._pgbuf.read_int32()
          sql = self._pgbuf.read_string(msglen - 4)
          self.query(sql)
        elif type_code == 'X':
          break
    except Exception as e:
      self.error(e.message)
    self.debug('disconnected')

  def read_ssl_request(self):
    msglen = self._pgbuf.read_int32()
    sslcode = self._pgbuf.read_int32()
    if msglen != 8 and sslcode != 80877103:
      raise Exception("Unsupported SSL request")

  def read_startup_message(self):
    msglen = self._pgbuf.read_int32()
    version = self._pgbuf.read_int32()
    v_maj = version >> 16
    v_min = version & 0xffff
    info = self._pgbuf.read_parameters(msglen - 8)
    self.debug('PSQL %d.%d' % (v_maj, v_min), str(info))

  def read_authentication(self):
    type_code = self._pgbuf.read_byte()
    if type_code != "p":
      self.send_error("FATAL", "28000", "authentication failure")
      raise Exception("Only 'Password' auth is supported, got %r" % type_code)

    msglen = self._pgbuf.read_int32()
    password = self._pgbuf.read_bytes(msglen - 4)
    print password

  def send_notice(self):
    self.wfile.write('N')

  def send_authentication_request(self):
    self.wfile.write(struct.pack("!cii", 'R', 8, 3))

  def send_authentication_ok(self):
    self.wfile.write(struct.pack("!cii", 'R', 8, 0))

  def send_ready_for_query(self):
    self.wfile.write(struct.pack("!cic", 'Z', 5, 'I'))

  def send_command_complete(self, tag):
    self.wfile.write(struct.pack("!ci", 'C', 4 + len(tag)))
    self.wfile.write(tag)

  def send_error(self, severity, code, message):
    buf = PgBuffer(StringIO())
    buf.write_byte('S')
    buf.write_string(severity)
    buf.write_byte('C')
    buf.write_string(code)
    buf.write_byte('M')
    buf.write_string(message)
    buf = buf.buffer.getvalue()

    self.wbuf.write_byte('E')
    self.wbuf.write_int32(4 + len(buf) + 1)
    self.wbuf.write_string(buf)

  def send_row_description(self, fields):
    buf = PgBuffer(StringIO())
    for field in fields:
      buf.write_string(field)
      buf.write_int32(0)    # Table ID
      buf.write_int16(0)    # Column ID
      buf.write_int32(0)    # object ID of the field's data type
      buf.write_int16(-1)   # data type size
      buf.write_int32(-1)   # type modifier
      buf.write_int16(0)    # text format code
    buf = buf.buffer.getvalue()

    self.wbuf.write_byte('T')
    self.wbuf.write_int32(6 + len(buf))
    self.wbuf.write_int16(len(fields))
    self.wbuf.write_bytes(buf)

  def send_row_data(self, rows):
    for row in rows:
      buf = PgBuffer(StringIO())
      for field in row:
        v = '%r' % field
        buf.write_int32(len(v))
        buf.write_bytes(v)
      buf = buf.buffer.getvalue()

      self.wbuf.write_byte('D')
      self.wbuf.write_int32(6 + len(buf))
      self.wbuf.write_int16(len(row))
      self.wbuf.write_bytes(buf)

  def execute_select(self, stmt):
    results = self.engine.execute_statement(stmt)
    #fields = [IntField('a'), IntField('b')]
    #rows = [[1, 2], [3, 4], [5, 6]]

    self.send_row_description(results.columns)
    self.send_row_data(results.rows)

  def execute_statement(self, stmt):
    self.engine.execute_statement(stmt)

  def query(self, sql):
    self.debug('query', repr(sql))

    try:
      queries = self.engine.parse(sql)
    except Exception as e:
      self.error('query parsing', e.message)
      self.send_error("ERROR", "26000", "query parse failure: %s" % e.message)
      return

    for stmt in queries:
      try:
        st = time()
        if isinstance(stmt, SqlSelectStatement):
          self.execute_select(stmt)
        else:
          self.execute_statement(stmt)
        et = time()
        self.send_command_complete('%s %.3fsec\x00' % (stmt.COMMAND, et - st))
        self.debug('query time', '%s completed in %.3fsec' % (stmt.COMMAND, et - st))
      except Exception as e:
        print e
        self.error('query processing', e.message)
        self.send_error("ERROR", "XX000", "query processing failure: %s" % e.message)

  def debug(self, *messages):
    logger.debug('%s - %s' % (str(self.client_address), ' - '.join(messages)))

  def error(self, *messages):
    logger.error('%s - %s' % (str(self.client_address), ' - '.join(messages)))

#class TcpServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
class TcpServer(SocketServer.ForkingMixIn, SocketServer.TCPServer):
  allow_reuse_address = True

if __name__ == '__main__':
  HOST = ''
  PORT = 55432

  logger = logging.getLogger('psql-server')
  logger.setLevel(logging.DEBUG)

  handler = logging.StreamHandler()
  handler.setLevel(logging.DEBUG)
  handler.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
  logger.addHandler(handler)

  server = TcpServer((HOST, PORT), PsqlHandler)
  logger.info('server running, try: $ psql -h localhost -p 55432')
  try:
    server.serve_forever()
  except:
    server.shutdown()
