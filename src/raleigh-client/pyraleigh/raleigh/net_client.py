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

from iopoll import IOPollEntity
from util import humanSize

class NetIOClient(IOPollEntity):
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
  def connection(self, iopoll, host, port):
    self.connect(host, port)
    iopoll.add(self)
    self._online = time()
    try:
      yield
    finally:
      t = time() - self._online
      iopoll.remove(self)
      self.disconnect()
      print 'tx %s %s/sec' % (humanSize(self.txbytes), humanSize(self.txbytes / t))
      print 'rx %s %s/sec' % (humanSize(self.rxbytes), humanSize(self.rxbytes / t))

  def connect(self, host, port):
    self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self._sock.connect((host, port))
    self._sock.setblocking(0)

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

  def send(self, data):
    self._wlock.acquire()
    self._wbuf += data
    self._wlock.release()
    self.set_writable()
    return len(data)

  def fetch(self):
    self._rlock.acquire()
    data = self._rbuf
    self._rlock.release()
    return data

  def pop(self, n):
    self._rlock.acquire()
    data = self._rbuf
    self._rbuf = data[n:]
    self._rlock.release()
    return data[:n]

  def write(self):
    self._wlock.acquire()
    try:
      data = self._wbuf
      if len(data) > 0:
        self._wbuf = bytearray()
        n = self._sock.send(data)
        if n < len(data):
          self._wbuf = data[n:] + self._wbuf
        self.txbytes += n
      if len(self._wbuf) == 0:
        self.set_writable(False)
    finally:
      self._wlock.release()

  def read(self):
    data = ''
    while self._sock != None:
      try:
        buf = self._sock.recv(4096)
        if not buf: break
        data += buf
      except:
        break
    self.rxbytes += len(data)
    self._rlock.acquire()
    self._rbuf += data
    self._rlock.release()
