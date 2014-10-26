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

from rpc_coding import FieldReader
from net_client import NetIOClient
from coding import *

import coding
import re

class IpcMsgClient(NetIOClient):
  REPLY_MAX_WAIT = 5

  def __init__(self, *args, **kwargs):
    super(IpcMsgClient, self).__init__(*args, **kwargs)
    self._req_id = 0

  def send_message(self, msg_type, body, blob):
    req_id = self._req_id
    self._req_id += 1
    msghead = self._encode_rpc_head(req_id, msg_type, len(body), len(blob))
    #self.send(msghead + body + blob)
    from time import sleep
    self.send(msghead)
    #if body: sleep(10)
    self.send(body)
    #if blob: sleep(10)
    self.send(blob)
    return req_id

  def recv_message(self):
    ibuf = self.fetch()
    while len(ibuf) >= 4:
      head = self._decode_rpc_head(ibuf)
      if not head:
        break

      head_size, h_pkt_type, msg_type, msg_id, body_len, blob_len = head
      length = (head_size + body_len + blob_len)
      if len(ibuf) < length:
        break

      data = self.pop(length)
      body = data[head_size:head_size+body_len]
      blob = data[head_size+body_len:head_size+body_len+blob_len]
      yield h_pkt_type, msg_type, msg_id, body, blob
      ibuf = self.fetch()

  def recv_message_wait(self):
    loop = True
    st = time()
    while loop:
      for r in self.recv_message():
        loop = False
        yield r

      if loop and time() - st > self.REPLY_MAX_WAIT:
        raise Exception("Waited too long %.2fsec" % (time() - st))

  def _sync_recv(self, fields):
    for h_pkt_type, msg_type, msg_id, body, blob in self.recv_message_wait():
      field_reader = FieldReader(fields, body, blob)
      return h_pkt_type, msg_type, msg_id, field_reader.parse()

  def _encode_rpc_head(self, msg_id, msg_type, body_len, blob_len):
    version = 0
    req_type = 1

    h_pkt_type = 0
    h_msg_type = z_uint_bytes(msg_type)
    h_msg_id   = z_uint_bytes(msg_id)
    h_body_len = z_uint_bytes(body_len) if body_len > 0 else 0
    h_blob_len = z_uint_bytes(blob_len) if blob_len > 0 else 0
    head_size  = 2 + h_msg_type + h_msg_id + h_body_len + h_blob_len

    head = bytearray(2)
    head[0] = h_pkt_type | (h_msg_type - 1)
    head[1] = ((h_msg_id - 1) << 5) | (h_body_len << 3) | h_blob_len

    head += z_encode_uint(h_msg_type, msg_type)
    head += z_encode_uint(h_msg_id, msg_id)
    head += z_encode_uint(h_body_len, body_len)
    head += z_encode_uint(h_blob_len, blob_len)
    print h_msg_type, msg_type, h_msg_id, msg_id
    print 'SND HEAD', ['%d' % c for c in head]
    assert len(head) == head_size
    return head

  def _decode_rpc_head(self, data):
    h_pkt_type = z_fetch_6bit(data[0], 2)
    h_msg_type = z_fetch_2bit(data[0], 0) + 1
    h_msg_id   = z_fetch_3bit(data[1], 5) + 1
    h_body_len = z_fetch_2bit(data[1], 3)
    h_blob_len = z_fetch_3bit(data[1], 0)
    head_size  = 2 + h_msg_type + h_msg_id + h_body_len + h_blob_len
    print 'RCV HEAD', data[0], data[1], ['%d' % c for c in data]
    if len(data) < head_size:
      return None

    msg_type = z_decode_uint(data[2:], h_msg_type)
    msg_id   = z_decode_uint(data[2 + h_msg_type:], h_msg_id)
    body_len = z_decode_uint(data[2 + h_msg_type + h_msg_id:], h_body_len)
    blob_len = z_decode_uint(data[2 + h_msg_type + h_msg_id + h_body_len:], h_blob_len)
    return head_size, h_pkt_type, msg_type, msg_id, body_len, blob_len
