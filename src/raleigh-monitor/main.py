#!/usr/bin/env python
#
# Copyright (c) 2012-2013, Matteo Bertozzi
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#   * Neither the name of the <organization> nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from http import HttpRequestHandler, UnixHttpServer, TcpHttpServer
from service import AbstractService

from collections import deque
from datetime import datetime
from time import time
import os
import re

import sys
sys.path.append("../raleigh-client/pyraleigh/")

from raleigh.client import StatsClient
from raleigh.iopoll import IOPoll

def _date_now(t):
  return datetime.strftime(datetime.fromtimestamp(t), '%d-%b-%y-%H:%M:%S')

def _pair_usage(usage):
  count = 0
  uprev = None
  for t, u in usage:
    if count > 1:
      yield t, uprev, u
    uprev = u
    count += 1

def _scale_down(value, old_range, new_range):
  return (value * new_range) / old_range

def _data_cpu(rusage):
  cpu_time = []
  for t, a, b in _pair_usage(rusage):
    cpu_time.append({'date': _date_now(t),
                     'user': b['utime'] - a['utime'],
                     'sys':  b['stime'] - a['stime'],
                    })
  return cpu_time

def _data_ctxswitch(rusage):
  data = []
  for t, a, b in _pair_usage(rusage):
    data.append({'date':   _date_now(t),
                 'nvcsw':  b['nvcsw'] - a['nvcsw'],
                 'nivcsw': b['nivcsw'] - a['nivcsw'],
                })
  return data

def _data_iopoll(iousage):
  data = []
  for t, a, b in _pair_usage(iousage):
    tdata = {'date': _date_now(t)}
    for i, (a_read, b_read) in enumerate(zip(a['reads'], b['reads'])):
      tdata['ioread[%d]' % i] = b_read - a_read
    for i, (a_write, b_write) in enumerate(zip(a['writes'], b['writes'])):
      tdata['iowrite[%d]' % i] = b_write - a_write
    data.append(tdata)
  return data

def _data_io(rusage):
  data = []
  for t, a, b in _pair_usage(rusage):
    data.append({'date':    _date_now(t),
                 'inblock':  (b['inblock'] - a['inblock']),
                 'outblock':  (b['outblock'] - a['outblock']),
                })
  return data

def _data_mem(memusage):
  data = []
  MB = 1024 * 1024.0
  for t, x in memusage:
    data.append({'date':    _date_now(t),
                 'sysused':  x['sysused'] / MB,
                 'sysfree':  x['sysfree'] / MB,
                })
  return data

def _data_malloc(memusage):
  data = []
  MB = 1024 * 1024.0
  for t, x in memusage:
    data.append({'date':    _date_now(t),
                 'hblkhd':   x['hblkhd']   / MB,
                 'usmblks':  x['usmblks']  / MB,
                 'fsmblks':  x['fsmblks']  / MB,
                 'uordblks': x['uordblks'] / MB,
                 'fordblks': x['fordblks'] / MB,
                })
  return data

_global_data = {}
_rusage = deque(maxlen=200)
_memusage = deque(maxlen=200)
_iousage = deque(maxlen=50)
_running = True

def __stats_update():
  from time import sleep
  iopoll = IOPoll()
  with iopoll.loop():
    while _running:
      try:
        client = StatsClient()
        with client.connection(iopoll, '127.0.0.1', 11217):
          while _running:
            _rusage.append((time(), client.rusage()))
            _global_data['cpu'] = _data_cpu(_rusage)
            _global_data['ctxswitch'] = _data_ctxswitch(_rusage)
            _global_data['io'] = _data_io(_rusage)

            _memusage.append((time(), client.memusage()))
            _global_data['mem'] = _data_mem(_memusage)
            _global_data['malloc'] = _data_malloc(_memusage)

            _iousage.append((time(), client.iopoll()))
            _global_data['iopoll'] = _data_iopoll(_iousage)

            sleep(1)
      except Exception as e:
        print e
        sleep(1)

class MonitorRequestHandler(HttpRequestHandler):
  RAW_FILE_EXT = ('htm', 'html', 'css' 'js', 'txt', 'png', 'jpg')
  INDEX_EXT = ('htm', 'html', 'py', 'txt')

  RESOURCES_DIR = 'resources'
  PAGES_DIR = '.'

  @HttpRequestHandler.match('/data-(.*).json')
  def _json_cpu(self, name):
    import json
    data = _global_data.get(name)
    return self.send_data(200, 'text/json', json.dumps(data))

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
