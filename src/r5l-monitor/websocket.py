#!/usr/bin/env python
#
#   Copyright 2014 Matteo Bertozzi
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

from SocketServer import StreamRequestHandler, ThreadingTCPServer
from struct import pack, unpack
from cStringIO import StringIO
from mimetools import Message
from base64 import b64encode
from select import select
from hashlib import sha1

WEBSOCKET_MAGIC_STR = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11'

class WebSocketHandler(StreamRequestHandler):
  READ_TIMEOUT = 2.0

  def setup(self):
    StreamRequestHandler.setup(self)
    print "connection established", self.client_address
    self._handshake_done = False

  def handle(self):
    while self.server.is_running():
      if not self._handshake_done:
        self._handshake()
      else:
        fd = self.rfile.fileno()
        r, _, _ = select([fd], [], [], self.READ_TIMEOUT)
        if fd in r:
          self._read_next_message()
        else:
          self.on_read_timeout()

  def on_message(self, message):
    pass

  def on_read_timeout(self):
    pass

  def send_message(self, message):
    self.request.send(chr(129))
    length = len(message)
    if length <= 125:
      self.request.send(chr(length))
    elif length >= 126 and length <= 65535:
      self.request.send(chr(126))
      self.request.send(pack(">H", length))
    else:
      self.request.send(chr(127))
      self.request.send(pack(">Q", length))
    self.request.send(message)

  def _handshake(self):
    data = self.request.recv(1024).strip()
    headers = Message(StringIO(data.split('\r\n', 1)[1]))
    if headers.get("Upgrade", None) != "websocket":
      return
    print 'Handshaking...'
    key = headers['Sec-WebSocket-Key']
    digest = b64encode(sha1(key + WEBSOCKET_MAGIC_STR).hexdigest().decode('hex'))
    response = 'HTTP/1.1 101 Switching Protocols\r\n'
    response += 'Upgrade: websocket\r\n'
    response += 'Connection: Upgrade\r\n'
    response += 'Sec-WebSocket-Accept: %s\r\n\r\n' % digest
    self._handshake_done = self.request.send(response)

  def _read_next_message(self):
    length = ord(self.rfile.read(2)[1]) & 127
    if length == 126:
      length = unpack(">H", self.rfile.read(2))[0]
    elif length == 127:
      length = unpack(">Q", self.rfile.read(8))[0]
    masks = [ord(byte) for byte in self.rfile.read(4)]
    decoded = ""
    for char in self.rfile.read(length):
      decoded += chr(ord(char) ^ masks[len(decoded) % 4])
    self.on_message(decoded)

class WebSocketTcpServer(ThreadingTCPServer):
  allow_reuse_address = True
