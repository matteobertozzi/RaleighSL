#!/usr/bin/env python
#
#   Copyright 2011-2013 Matteo Bertozzi
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

from contextlib import contextmanager
from time import time

import threading
import socket

from util import humanSize

class NetIOClient(object):
  def __init__(self):
    super(NetIOClient, self).__init__()
    self._wlock = threading.Lock()
    self._rlock = threading.Lock()
    self._rbuf = bytearray()
    self._wbuf = bytearray()
    self._sock = None
    self.rxbytes = 0
    self.txbytes = 0

  @contextmanager
  def connection(self, iopoll, host, port, dump_stats=False):
    self.connect(host, port)
    self._online = time()
    try:
      yield
    finally:
      t = time() - self._online
      self.disconnect()
      if dump_stats:
        tx_info = 'tx %s %s/sec' % (humanSize(self.txbytes), humanSize(self.txbytes / t))
        rx_info = 'rx %s %s/sec' % (humanSize(self.rxbytes), humanSize(self.rxbytes / t))
        print '%s - %s' % (tx_info, rx_info)

  def connect(self, host, port):
    self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self._sock.connect((host, port))

  def disconnect(self):
    if self._sock:
      self._sock.close()
      self._sock = None

  def close(self):
    self.disconnect()

  def is_closed(self):
    return self._sock == None

  def set_no_delay(self, no_delay=True):
    self._sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, no_delay)

  def fileno(self):
    return self._sock.fileno() if self._sock else -1

  def is_readable(self):
    return self._sock is not None
  def is_writable(self):
    return self._sock is not None
  def is_watched(self):
    return False

  def send(self, data):
    self._sock.sendall(data)

  def pop(self, n):
    if not self._rbuf:
      self.fetch()
    data = self._rbuf[:n]
    self._rbuf = self._rbuf[n:]
    return data

  def fetch(self):
    if self._rbuf:
      from select import select
      from time import time
      st = time()
      r, w, e = select([self._sock], [], [], 0.1)
      if r:
        self._rbuf += self._sock.recv(4096)
      et = time()
      if (et - st) > 100:
        print 'wait %.3fsec for %r' % (et - st, r)
    else:
      self._rbuf += self._sock.recv(4096)
    return self._rbuf
