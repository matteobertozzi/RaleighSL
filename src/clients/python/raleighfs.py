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

import socket

import coding

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

class NetClient(INetClient):
    def __init__(self):
        self._sock = None

    def connect(self, host, port):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)
        self._sock.connect((host, port))

    def disconnect(self):
        self._sock.close()
        self._sock = None

    def send(self, data):
        #return self._sock.send(data)
        return self._sock.sendall(data)

    def recv(self, length=8192):
        return self._sock.recv(length)

    def recvFully(self, length):
        data = []
        while length > 0:
            chunk = self._sock.recv(length)
            length -= len(chunk)
            data.append(chunk)
        return ''.join(data)

class RaleighFS(INetClient):
    def __init__(self):
        self._client = NetClient()
        self._ibuf = bytearray()
        self._txbytes = 0
        self._rxbytes = 0
        self._online = 0

    def connect(self, host, port):
        self._client.connect(host, port)
        self._online = time()

    def disconnect(self):
        t = time() - self._online
        self._client.disconnect()

        print 'tx %s %s/sec' % (humanSize(self._txbytes), humanSize(self._txbytes / t))
        print 'rx %s %s/sec' % (humanSize(self._rxbytes), humanSize(self._rxbytes / t))

    def send(self, msgid, data):
        assert len(data), 'Message data must be at least 1 byte!'
        buf = str(coding.z_encode_field(msgid, len(data))) + data
        self._client.send(buf)
        self._txbytes += len(buf)
        return len(buf)

    def recv(self):
        data = self._client.recv()
        self._ibuf.extend(bytearray(data))
        self._rxbytes += len(data)
        ibuf = self._ibuf

        while len(ibuf) > 0:
            elen, field_id, length = coding.z_decode_field(ibuf)
            if elen < 0: break
            yield field_id, ibuf[elen:elen+length]
            self._ibuf = ibuf[elen+length:]
            ibuf = self._ibuf

def humanSize(size):
    hs = ('KiB', 'MiB', 'GiB', 'TiB', 'PiB')
    count = len(hs)
    while count > 0:
        ref = float(1 << (count * 10))
        count -= 1
        if size >= ref:
            return '%.2f%s' % (size / ref, hs[count])
    return '%dbytes' % size