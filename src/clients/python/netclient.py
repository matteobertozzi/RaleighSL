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

class INetClient(object):
  def connect(self, host, port):
    raise NotImplementedError

  def disconnect(self):
    raise NotImplementedError

  @contextmanager
  def connection(self, host, port):
    self.connect(host, port)
    try:
      yield
    finally:
      self.disconnect()

class NetIOClient(INetClient):
  def __init__(self):
    self._wlock = threading.Lock()
    self._rlock = threading.Lock()
    self._rbuf = bytearray()
    self._wbuf = bytearray()
    self._sock = None
    self.rxbytes = 0
    self.txbytes = 0

  def connect(self, host, port):
    self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self._sock.connect((host, port))
    self._sock.setblocking(0)

  def disconnect(self):
    self._sock.close()
    self._sock = None

  def set_no_delay(self, no_delay=True):
    self._sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, no_delay)

  def fileno(self):
    return self._sock.fileno() if self._sock else -1

  def send(self, data):
    self._wlock.acquire()
    self._wbuf += data
    self._wlock.release()
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

  def is_writable(self):
    return len(self._wbuf) > 0

  def write(self):
    self._wlock.acquire()
    data = self._wbuf
    if len(data) > 0:
      self._wbuf = bytearray()
    self._wlock.release()
    if len(data) > 0:
      n = self._sock.send(data)
      if n < len(data):
        self._wlock.acquire()
        self._wbuf = data[n:] + self._wbuf
        self._wlock.release()
      self.txbytes += n

  def read(self):
    data = self._sock.recv(4096)
    if len(data) >= 4096:
      while True:
        buf = self._sock.recv(4096)
        if not buf: break
        data += buf
    self.rxbytes += len(data)
    self._rlock.acquire()
    self._rbuf += data
    self._rlock.release()

class NetIOClientWrapper(INetClient):
    def __init__(self):
      self._client = NetIOClient()
      self._online = 0

    def connect(self, host, port):
      self._client.connect(host, port)
      self._online = time()

    def disconnect(self):
      t = time() - self._online
      self._client.disconnect()

      txbytes = self._client.txbytes
      rxbytes = self._client.rxbytes
      print 'tx %s %s/sec' % (humanSize(txbytes), humanSize(txbytes / t))
      print 'rx %s %s/sec' % (humanSize(rxbytes), humanSize(rxbytes / t))

    txbytes = property(lambda x: x._client.txbytes)
    rxbytes = property(lambda x: x._client.rxbytes)

    def fileno(self):
      return self._client.fileno()

    def is_writable(self):
      return self._client.is_writable()

    def read(self):
      return self._client.read()

    def write(self):
        return self._client.write()

def humanSize(size):
  hs = ('KiB', 'MiB', 'GiB', 'TiB', 'PiB')
  count = len(hs)
  while count > 0:
    ref = float(1 << (count * 10))
    count -= 1
    if size >= ref:
      return '%.2f%s' % (size / ref, hs[count])
  return '%dbytes' % size