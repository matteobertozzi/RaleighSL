#!/usr/bin/env python
#
#   Copyright 2013 Matteo Bertozzi
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

from http import HttpRequestHandler, UnixHttpServer, TcpHttpServer
from service import AbstractService
import charts

from collections import deque
import re
import os

import sys
sys.path.append("../clients/python/")

_rusage = deque(maxlen=100)
_running = True

def __stats_update():
  from time import time, sleep
  from raleighfs import RaleighFS, StatsClient
  from netclient import humanSize
  from iopoll import IOPoll

  iopoll = IOPoll()
  iopoll.start()
  try:
    while _running:
      try:
        client = StatsClient()
        with client.connection('127.0.0.1', 11217):
          iopoll.add(client)
          while _running:
            client.send('x')
            count = 0
            while count < 1 and _running:
              for data in client.recv_struct():
                _rusage.append((int(time() * 1000), data))
                count += 1
            sleep(1)
      except:
        pass
  finally:
    iopoll.stop()

def _pair_rusage():
  count = 0
  uprev = None
  for t, u in _rusage:
    if count > 1:
      yield t, uprev, u
    uprev = u
    count += 1

class MonitorRequestHandler(HttpRequestHandler):
  RAW_FILE_EXT = ('htm', 'html', 'css' 'js', 'txt', 'png', 'jpg')
  INDEX_EXT = ('htm', 'html', 'py', 'txt')

  RESOURCES_DIR = '.'
  PAGES_DIR = '.'

  @HttpRequestHandler.match('/cpu.js')
  def _cpu(self):
    cpu_time = charts.StackedAreaChart('cpu', 250)
    cpu_time.add('user', [(t, b.utime - a.utime) for t, a, b in _pair_rusage()])
    cpu_time.add('sys', [(t, b.stime - a.stime) for t, a, b in _pair_rusage()])
    return self.send_data(200, 'text/json', cpu_time.toJsonData())

  @HttpRequestHandler.match('/mem.js')
  def _mem(self):
    mem = charts.StackedAreaChart('mem', 250)
    mem.add('maxrss', [(t, u.maxrss) for t, u in _rusage])
    return self.send_data(200, 'text/json', mem.toJsonData())

  @HttpRequestHandler.match('/ctxswitch.js')
  def _ctxswitch(self):
    ctxswitch = charts.StackedAreaChart('switch', 250)
    ctxswitch.add('nvcsw', [(t, b.nvcsw - a.nvcsw) for t, a, b in _pair_rusage()])
    ctxswitch.add('nivcsw', [(t, b.nivcsw - a.nivcsw) for t, a, b in _pair_rusage()])
    return self.send_data(200, 'text/json', ctxswitch.toJsonData())

  @HttpRequestHandler.match('/iopoll.js')
  def _iopoll(self):
    iopoll = charts.StackedAreaChart('iopoll', 250)
    iopoll.add('iowait', [(t, b.iowait - a.iowait) for t, a, b in _pair_rusage()])
    iopoll.add('ioread', [(t, b.ioread - a.ioread) for t, a, b in _pair_rusage()])
    iopoll.add('iowrite', [(t, b.iowrite - a.iowrite) for t, a, b in _pair_rusage()])
    return self.send_data(200, 'text/json', iopoll.toJsonData())

  @HttpRequestHandler.match('/(.*)')
  def page(self, name):
    path = self._find_path(name)
    if path is None:
        self.handle_not_found()
    else:
        self.send_file(path)

  def _find_path(self, name):
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

class MonitoringService(AbstractService):
  CLS_REQUEST_HANDLER = MonitorRequestHandler
  CLS_UNIX_SERVER = UnixHttpServer
  CLS_TCP_SERVER = TcpHttpServer

if __name__ == '__main__':
  import threading

  threading.Thread(target=__stats_update).start()

  BIND_ADDRESS = ('', 8080)
  service = MonitoringService()
  try:
    service.run(BIND_ADDRESS)
  finally:
    print 'quitting'
    _running = False

