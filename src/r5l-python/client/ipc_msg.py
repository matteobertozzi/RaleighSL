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

from coding import z_uint_bytes, z_encode_uint, z_decode_uint

import socket

# +------------------+
# | ---- |  11 |  11 | pkg-type, msg-type, fwd
# |  111 |  11 | 111 | msg-id, body, blob
# +------------------+
# |     msg type     | (min 1, max 4) 0b11
# |     msg id       | (min 1, max 8) 0b111
# +------------------+
# |    fwd length    | (min 0, max 3) 0b11
# +------------------+
# |   body length    | (min 0, max 3) 0b11
# +------------------+
# |   data length    | (min 0, max 4) 0b111
# +------------------+
# /     fwd blob     /
# +------------------+
# /    body blob     /
# +------------------+
# /    data blob     /
# +------------------+
def build_msg_head(pkg_type, msg_type, msg_id, fwd, body, data):
  msg_id_lbytes = z_uint_bytes(msg_id)
  msg_type_lbytes = z_uint_bytes(msg_type)
  fwd_lbytes = z_uint_bytes(len(fwd)) if fwd else 0
  body_lbytes = z_uint_bytes(len(body)) if body else 0
  data_lbytes = z_uint_bytes(len(data)) if data else 0

  head = bytearray(2)
  head[0] = (pkg_type << 4) | ((msg_type_lbytes - 1) << 2) | (fwd_lbytes)
  head[1] = ((msg_id_lbytes - 1) << 5) | (body_lbytes << 3) | (data_lbytes)
  head += z_encode_uint(msg_type_lbytes, msg_type)
  head += z_encode_uint(msg_id_lbytes, msg_id)
  head += z_encode_uint(fwd_lbytes, len(fwd) if fwd else 0)
  head += z_encode_uint(body_lbytes, len(body) if body else 0)
  head += z_encode_uint(data_lbytes, len(data) if data else 0)
  return head

class IncompleteIpcMsgError(Exception):
  pass

def parse_msg_head(buf):
  if len(buf) < 2: raise IncompleteIpcMsgError()
  pkg_type = (buf[0] >> 4) & 15
  msg_type_lbytes = ((buf[0] >> 2) & 3) + 1
  fwd_lbytes = (buf[0] & 3)
  msg_id_lbytes = ((buf[1] >> 5) & 7) + 1
  body_lbytes = (buf[1] >> 3) & 3
  data_lbytes = (buf[1] & 7)

  hlen  = 2 + msg_type_lbytes + msg_id_lbytes
  hlen += fwd_lbytes + body_lbytes + data_lbytes
  if len(buf) < hlen: raise IncompleteIpcMsgError()

  offset = 2
  msg_type = z_decode_uint(buf, msg_type_lbytes, offset); offset += msg_type_lbytes
  msg_id   = z_decode_uint(buf, msg_id_lbytes, offset);   offset += msg_id_lbytes
  fwd_len  = z_decode_uint(buf, fwd_lbytes, offset);      offset += fwd_lbytes
  body_len = z_decode_uint(buf, body_lbytes, offset);     offset += body_lbytes
  data_len = z_decode_uint(buf, data_lbytes, offset);     offset += data_lbytes

  return pkg_type, msg_type, msg_id, fwd_len, body_len, data_len, offset

class IpcClient(object):
  def __init__(self):
    self._sock = None
    self._msg_id = 0

  def connect(self, host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    self._sock = sock

  def disconnect(self):
    self._sock.close()
    self._sock = None

  def _next_msg_id(self):
    msg_id = self._msg_id
    self._msg_id += 1
    return msg_id

  def send_msg(self, pkg_type, msg_type, fwd=None, body=None, data=None):
    sock = self._sock
    msg_id = self._next_msg_id()
    head = build_msg_head(pkg_type, msg_type, msg_id, fwd, body, data)
    sock.sendall(head)
    if fwd: sock.sendall(fwd)
    if body: sock.sendall(body)
    if data: sock.sendall(data)

  def recv_msg(self):
    buf = bytearray(4096)
    rd = self._sock.recv_into(buf)
    buf = buf[:rd]

    head = parse_msg_head(buf)
    fwd_len  = head[3]
    body_len = head[4]
    data_len = head[5]
    offset   = head[6]

    while rd < (offset + fwd_len + body_len + data_len):
      buf += self._sock.recv(1024)

    fwd  = buf[offset:offset+fwd_len];  offset += fwd_len
    body = buf[offset:offset+body_len]; offset += body_len
    data = buf[offset:offset+data_len]
    return head[:3], fwd, body, data
