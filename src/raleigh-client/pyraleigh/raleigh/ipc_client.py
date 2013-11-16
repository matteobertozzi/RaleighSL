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

from time import time

from net_client import NetIOClient
from coding import *

import coding
import re

class IpcFramedClient(NetIOClient):
  REPLY_MAX_WAIT = 5

  def __init__(self):
    super(IpcFramedClient, self).__init__()

  def send(self, data):
    buf = self._encode_head(data) + data
    return super(IpcFramedClient, self).send(buf)

  def recv(self):
    ibuf = self.fetch()
    while len(ibuf) > 8:
      length = self._decode_head(ibuf)
      if length > len(ibuf): break
      data = self.pop(length)
      yield data[8:]
      ibuf = self.fetch()

  def recv_wait(self):
    loop = True
    st = time()
    while loop and not self.is_closed():
      for r in self.recv():
        loop = False
        yield r

      if loop and time() - st > self.REPLY_MAX_WAIT:
        raise Exception("Waited too long %.2fsec" % (time() - st))

  def _encode_head(self, data):
    head = bytearray(8)
    head[0] = 0
    head[1] = len(data) & 0xff
    head[2] = (len(data) >> 8) & 0xff
    head[3] = (len(data) >> 16) & 0xff
    head[4] = 0xd5
    head[5] = 0x33
    head[6] = 0xcc
    head[7] = 0xaa
    return head

  def _decode_head(self, ibuf):
    assert len(ibuf) >= 8
    version = ibuf[0]
    size    = ibuf[1] | ibuf[2] << 8 | ibuf[3] << 16
    magic   = ibuf[4] | ibuf[5] << 8 | ibuf[6] << 16 | ibuf[7] << 24
    if version != 0: raise Exception("Invalid Version %d" % version)
    if magic != 0xaacc33d5: raise Exception("Invalid Magic %x" % magic)
    return 8 + size

class IpcRpcClient(IpcFramedClient):
  def __init__(self, *args, **kwargs):
    super(IpcRpcClient, self).__init__(*args, **kwargs)
    self._req_id = 0

  def send_message(self, msg_type, data):
    req_id = self._req_id
    self._req_id += 1
    msghead = self._encode_rpc_head(req_id, msg_type)
    self.send(msghead + data)
    return req_id

  def recv_message(self):
    for msg in self.recv():
      req_id, req_type, data = self._decode_rpc_head(msg)
      yield req_id, req_type, data

  def recv_message_wait(self):
    loop = True
    st = time()
    while loop:
      for r in self.recv_message():
        loop = False
        yield r

      if loop and time() - st > self.REPLY_MAX_WAIT:
        raise Exception("Waited too long %.2fsec" % (time() - st))

  def _encode_rpc_head(self, req_id, msg_type):
    len_a = z_uint_bytes(msg_type);
    len_b = z_uint_bytes(req_id);
    head  = bytearray(1)
    head[0] = ((len_a - 1) << 5) | ((len_b - 1) << 2) | (1 << 1)
    head += z_encode_uint(len_a, msg_type)
    head += z_encode_uint(len_b, req_id)
    assert len(head) == (1 + len_a + len_b)
    return head

  def _decode_rpc_head(self, data):
    len_a = 1 + z_fetch_3bit(data[0], 5)
    len_b = 1 + z_fetch_3bit(data[0], 2)
    msg_type = z_decode_uint(data[1:], len_a)
    req_id = z_decode_uint(data[1 + len_a:], len_b)
    return req_id, msg_type, data[1 + len_a + len_b:]

class FieldStruct(object):
  _UNKNOWN_FIELD = (None, None, None)

  def __init__(self):
    self._values = {}

  def keys(self):
    return self._values.iterkeys()

  def __repr__(self):
    return str(self._values)

  def __getattr__(self, name):
    value = self._values.get(name)
    if value is None:
      raise AttributeError(name)
    return value

  @staticmethod
  def parse(data, fields):
    values = {}
    while len(data) > 0:
      elength, field, length = z_decode_field(data)
      data = data[elength:]
      field_name, field_type, _ = fields.get(field, FieldStruct._UNKNOWN_FIELD)
      if field_name is not None and field_type is not None:
        if isinstance(field_type, dict):
          values[field_name] = FieldStruct.parse(data[:length], field_type)
        elif field_type.startswith('list'):
          field_type = re.match('list\[(\w+)\]', field_type).groups()[0]
          func = getattr(coding, 'z_decode_' + field_type)
          values.setdefault(field_name, []).append(func(data, length))
        else:
          func = getattr(coding, 'z_decode_' + field_type)
          values[field_name] = func(data, length)
      data = data[length:]
    return values
