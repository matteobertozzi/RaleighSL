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

from ipc_client import IpcRpcClient
from ipc_client import FieldStruct
from coding import *

class RaleighException(Exception):
  def __init__(self, data):
    self.data = data
    self.code = data.get('code')
    self.message = data.get('message')

  def __str__(self):
    return 'ERROR %d: %s' % (self.code, self.message)

class RaleighClient(IpcRpcClient):
  STATUS_FIELDS = ('error', {1: ('code', 'uint', None), 2: ('message', 'string', None)}, None)
  DEFAULT_ERROR = {'code': 0}

  def semantic_open(self, object_name):
    data  = z_encode_field_bytes(1, object_name)
    self.send_message(10, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('oid', 'uint', None)})

  def semantic_create(self, object_name, object_type):
    data  = z_encode_field_bytes(1, object_name)
    data += z_encode_field_bytes(2, object_type)
    self.send_message(11, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('oid', 'uint', None)})

  def semantic_delete(self, object_name):
    data  = z_encode_field_bytes(1, object_name)
    self.send_message(12, data)
    return self._sync_recv({0: self.STATUS_FIELDS})

  def semantic_rename(self, old_name, new_name):
    data  = z_encode_field_bytes(1, old_name)
    data += z_encode_field_bytes(2, new_name)
    self.send_message(13, data)
    return self._sync_recv({0: self.STATUS_FIELDS})


  def transaction_create(self):
    self.send_message(20, '')
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('txn_id', 'uint', None)})

  def transaction_commit(self, txn_id):
    data  = z_encode_field_uint(1, txn_id)
    self.send_message(21, data)
    return self._sync_recv({0: self.STATUS_FIELDS})

  def transaction_rollback(self, txn_id):
    data  = z_encode_field_uint(1, txn_id)
    self.send_message(22, data)
    return self._sync_recv({0: self.STATUS_FIELDS})

  # ===========================================================================
  #  Counter
  # ===========================================================================
  def counter_get(self, oid, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(30, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('value', 'int', None)})

  def counter_set(self, oid, value, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_int(2, value)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(31, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('value', 'int', None)})

  def counter_cas(self, oid, old_value, new_value, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_int(2, old_value)
    data += z_encode_field_int(3, new_value)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(32, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('value', 'int', None)})

  def counter_inc(self, oid, txn_id=None):
    return self.counter_add(oid, 1, txn_id)

  def counter_dec(self, oid, txn_id=None):
    return self.counter_add(oid, -1, txn_id)

  def counter_add(self, oid, value, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_int(2, value)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(33, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('value', 'int', None)})

  def counter_mul(self, oid, value, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_int(2, value)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(34, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('value', 'int', None)})

  # ===========================================================================
  #  Sorted-Set
  # ===========================================================================
  def sset_insert(self, oid, key, value, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_bytes(3, key)
    data += z_encode_field_bytes(4, value)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(40, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('value', 'bytes', None)})

  def sset_update(self, oid, key, value, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_bytes(2, key)
    data += z_encode_field_bytes(3, value)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(41, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('old_value', 'bytes', None)})

  def sset_pop(self, oid, key, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_bytes(2, key)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(43, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('value', 'bytes', None)})

  def sset_get(self, oid, key, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_bytes(2, key)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(45, data)
    return self._sync_recv({0: self.STATUS_FIELDS, 1: ('value', 'bytes', None)})

  def sset_scan(self, oid, count, key=None, include_key=True, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_uint(2, count)
    data += z_encode_field_uint(4, int(include_key))
    if key: data += z_encode_field_bytes(3, key)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(46, data)
    return self._sync_recv({0: self.STATUS_FIELDS,
                            1: ('keys', 'list[bytes]', None),
                            2: ('values', 'list[bytes]', None)})

  # ===========================================================================
  #  Deque
  # ===========================================================================
  def deque_push(self, oid, value, front=True, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_uint(2, int(front))
    data += z_encode_field_bytes(3, value)
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(50, data)
    return self._sync_recv({0: self.STATUS_FIELDS})

  def deque_pop(self, oid, front=True, txn_id=None):
    data  = z_encode_field_uint(1, oid)
    data += z_encode_field_uint(2, int(front))
    if txn_id: data += z_encode_field_uint(0, txn_id)
    self.send_message(51, data)
    return self._sync_recv({0: self.STATUS_FIELDS,
                            1: ('data', 'bytes', None)})

  # ===========================================================================
  #  Server
  # ===========================================================================
  def ping(self):
    self.send_message(90, '')
    self._sync_recv({})

  def info(self):
    self.send_message(91, '')
    self._sync_recv({})

  def quit(self):
    self.send_message(92, '')
    self._sync_recv({})

  def _sync_recv(self, fields):
    for req_id, req_type, data in self.recv_message_wait():
      data = FieldStruct.parse(data, fields)
      error = data.pop('error', self.DEFAULT_ERROR)
      if error.get('code') != 0:
        raise RaleighException(error)
      return data
