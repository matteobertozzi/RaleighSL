#!/usr/bin/env python
#
#   Copyright 2011-2014 Matteo Bertozzi
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

from BaseHTTPServer import BaseHTTPRequestHandler
from SocketServer import ThreadingTCPServer
from httplib import HTTPConnection
from urlparse import parse_qsl

import socket
import urllib
import cgi
import re
import os

MIME_TABLE = {
  'js':   'application/x-javascript',
  'css':  'text/css',
  'png':  'image/png',
  'jpg':  'image/jpeg',
  'txt':  'text/plain',
  'htm':  'text/html',
  'html': 'text/html',
}

def _url_collapse_path(path):
  path_parts = path.split('/')
  head_parts = []
  for part in path_parts[:-1]:
    if part == '..':
      head_parts.pop()
    elif part and part != '.':
      head_parts.append( part )
  if path_parts:
    tail_part = path_parts.pop()
    if tail_part:
      if tail_part == '..':
        head_parts.pop()
        tail_part = ''
      elif tail_part == '.':
        tail_part = ''
  else:
    tail_part = ''
  return ('/' + '/'.join(head_parts), tail_part)

class HttpMatchRequest(object):
  def __init__(self, uri_pattern, commands):
    if not commands:
      self.commands = None
    else:
      if isinstance(commands, basestring):
        commands = [commands]
      self.commands = set(c.upper() for c in commands)
    self.uri_re = re.compile(uri_pattern)

  def __call__(self, uri, command):
    if self.commands is not None and command not in self.commands:
      return None
    m = self.uri_re.match(uri)
    if m is not None:
      return m.groups()
    return None

class HttpRequestHandler(BaseHTTPRequestHandler):
  RAW_FILE_EXT = ('htm', 'html', 'css' 'js', 'txt', 'png', 'jpg')
  INDEX_EXT = ('htm', 'html', 'py', 'txt')

  RESOURCES_DIR = '.'
  PAGES_DIR = '.'

  def __init__(self, *args, **kwargs):
    self.query = []
    self._static_rules = self._fetch_static_rules()
    BaseHTTPRequestHandler.__init__(self, *args, **kwargs)

  def send_headers(self, code, content_type, headers=None):
    if content_type:
      self.send_response(code)
      self.send_header('Content-Type', content_type)
    else:
      self.send_response(code, "Script output follows")
    if headers is not None:
      for key, value in headers.iteritems():
        self.send_header(key, value)
    self.end_headers()

  def send_file(self, filename, replaces=None):
    if os.path.exists(filename):
      self.send_headers(200, self.guess_mime(filename))
      fd = open(filename)
      try:
        if replaces:
          # TODO: Replace me with RegEx
          data = fd.read()
          for k, v in replaces:
            data = data.replace(k, v)
          self.wfile.write(data)
        else:
          while True:
            data = fd.read(8192)
            if not data: break
            self.wfile.write(data)
      finally:
        fd.close()
    else:
      self.log_message("File %s not found", filename)
      self.handle_not_found()

  def send_data(self, code, content_type, data):
    self.send_headers(code, content_type)
    self.wfile.write(data)

  def guess_mime(self, filename):
    _, ext = os.path.splitext(filename)
    return MIME_TABLE.get(ext[1:], 'application/octet-stream')

  def __getattr__(self, name):
    if name.startswith('do_'):
      return lambda: self._dispatch(name[3:].upper())
    raise AttributeError(name)

  def _dispatch(self, command):
    directory, name = _url_collapse_path(self.path)
    query_index = name.find('?')
    if query_index >= 0:
      self.query = parse_qsl(name[query_index+1:])
      name = name[:query_index]
    else:
      self.query = []

    self.path = os.path.join(directory, name)
    for match, handle_func in self._rules():
      result = match(self.path, command)
      if result is not None:
        try:
          handle_func(*result)
        except Exception, e:
          print e
          self.handle_failure(e)
        finally:
          break
    else:
      self.handle_not_found()

  def handle_failure(self, exception):
    self.send_error(500, "Internal Server Error")

  def handle_not_found(self):
    self.send_error(404, "File not found")

  def _post_data(self, maxlen=None):
    #'application/x-www-form-urlencoded':
    #curl -d "param1=value1&param2=value2" http://localhost:8080/

    # multipart/form-data
    #curl -F "fileupload=@test.txt" http://localhost:8080/

    if self.command.upper() != 'POST':
      return []

    ctype, pdict = cgi.parse_header(self.headers.typeheader or self.headers.type)
    if ctype == 'multipart/form-data':
      data = cgi.parse_multipart(self.rfile, pdict)
    elif ctype == 'application/x-www-form-urlencoded':
      clength = int(self.headers.getheader('content-length', 0))
      if maxlen is not None and clength > maxlen:
        raise ValueError, 'Maximum content length exceeded'
      d = self.rfile.read(clength)
      data = parse_qsl(d)
    else:
      data = []

    return data

  def _load_file(self, filename):
    fd = open(filename)
    try:
      data = fd.read()
    finally:
      fd.close()
    return data

  def _fetch_static_rules(self):
    return [(rule, method) for method in (getattr(self, attr)
                 for attr in dir(self))
                 for rule in getattr(method, "_rules", [])]

  def _rules(self):
    for rule in self._static_rules:
      yield rule

  def find_path(self, name):
    path = os.path.join(self.PAGES_DIR, name)
    if os.path.exists(path):
      rpath = self._find_file(path, True)
      if rpath is not None: return rpath
    path = self._find_file(path, False)
    if path is None:
      path = os.path.join(self.RESOURCES_DIR, name)
      path = self._find_file(path, os.path.exists(path))
    return path

  def _find_file(self, path, exists):
    if os.path.isdir(path):
      # Search for 'index*' file
      names = []
      nprio = len(self.INDEX_EXT)
      rx = re.compile('^index(\\..+|)$')
      for f in os.listdir(path):
        if not rx.match(f):
          continue
        _, ext = os.path.splitext(f)
        try:
          prio = self.INDEX_EXT.index(ext[1:])
        except ValueError:
          prio = nprio
        names.append((prio, f))
      if len(names) == 0:
        return None
      names.sort()
      return os.path.join(path, names[0][1])
    if not exists:
      for ext in self.INDEX_EXT:
        f = '%s.%s' % (path, ext)
        if os.path.exists(f):
          return f
      return None
    return path

  @classmethod
  def match(cls, uri_pattern, commands=None):
    rule = HttpMatchRequest(uri_pattern, commands)
    def _wrap(method):
      method._rules = getattr(method, '_rules', []) + [rule]
      return method
    return _wrap

class HttpTcpServer(ThreadingTCPServer):
  allow_reuse_address = True
