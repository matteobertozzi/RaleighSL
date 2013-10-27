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

from client import RaleighClient

class _RaleighObject(object):
  TYPE = None

  def __init__(self, client, oid):
    self._client = client
    self._oid = oid

  def __repr__(self):
    return '%s(%d)' % (self.TYPE, self._oid)

class RaleighTransaction(object):
  def __init__(self, client):
    self._client = client

  def begin(self):
    data = self._client.transaction_create()
    self.txn_id = data['txn_id']
    return data

  def commit(self):
    return self._client.transaction_commit(self.txn_id)

  def rollback(self):
    return self._client.transaction_rollback(self.txn_id)

class RaleighSSet(_RaleighObject):
  TYPE = 'sset'

  class Scanner:
    SCAN_SIZE = 10

    def __init__(self, sset):
      self._sset = sset
      self._last_key = None

    def __iter__(self):
      while True:
        data = self.next()
        keys = data.get('keys')
        if not keys: break
        values = data.get('values')
        for k, v in zip(keys, values):
          yield k, v

    def next(self, size=None):
      data = self._sset.scan(size or self.SCAN_SIZE, self._last_key, False)
      keys = data.get('keys')
      if keys: self._last_key = keys[-1]
      return data

  def insert(self, key, value, txn_id=None):
    return self._client.sset_insert(self._oid, key, value, txn_id)

  def update(self, key, value, txn_id=None):
    return self._client.sset_update(self._oid, key, value, txn_id)

  def pop(self, key, txn_id=None):
    return self._client.sset_pop(self._oid, key, txn_id)

  def get(self, key, txn_id=None):
    return self._client.sset_get(self._oid, key, txn_id)

  def scan(self, count, key=None, include_key=True, txn_id=None):
    return self._client.sset_scan(self._oid, count, key, include_key, txn_id)

  def create_scanner(self):
    return RaleighSSet.Scanner(self)

class RaleighCounter(_RaleighObject):
  TYPE = 'counter'

  def get(self, txn_id=None):
    return self._client.counter_get(self._oid, txn_id)

  def set(self, value, txn_id=None):
    return self._client.counter_set(self._oid, value, txn_id)

  def cas(self, old_value, new_value, txn_id=None):
    return self._client.counter_cas(self._oid, old_value, new_value, txn_id)

  def inc(self, txn_id=None):
    return self._client.counter_inc(self._oid, txn_id)

  def dec(self, txn_id=None):
    return self._client.counter_dec(self._oid, txn_id)

  def add(self, value, txn_id=None):
    return self._client.counter_add(self._oid, value, txn_id)

  def mul(self, value, txn_id=None):
    return self._client.counter_mul(self._oid, value, txn_id)

class RaleighDeque(_RaleighObject):
  TYPE = 'deque'

  def push_back(self, data, txn_id=None):
    return self._client.deque_push(self._oid, data, False, txn_id)

  def push_front(self, data, txn_id=None):
    return self._client.deque_push(self._oid, data, True, txn_id)

  def pop_back(self, txn_id=None):
    return self._client.deque_pop(self._oid, False, txn_id)

  def pop_front(self, txn_id=None):
    return self._client.deque_pop(self._oid, True, txn_id)
