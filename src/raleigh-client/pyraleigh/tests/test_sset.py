#!/usr/bin/env python
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

from hashlib import sha1
from bisect import insort, bisect_left

from raleigh.objects import RaleighSSet
from raleigh.objects import RaleighTransaction
from raleigh.client import RaleighException
from raleigh.test import RaleighTestCase

class TestSSet(RaleighTestCase):
  def test_simple(self):
    oid = self.createObject(RaleighSSet.TYPE)
    sset = RaleighSSet(self.client, oid)

    sset.insert('key-01', 'value-01')
    sset.insert('key-02', 'value-02')
    data = sset.get('key-01')
    self.assertEquals(data['value'], 'value-01')
    data = sset.get('key-02')
    self.assertEquals(data['value'], 'value-02')
    self.assertRaises(RaleighException, sset.get, 'key-03')

    data = sset.scan(2)
    self.assertEquals(data['keys'], ['key-01', 'key-02'])
    self.assertEquals(data['values'], ['value-01', 'value-02'])

    data = sset.pop('key-01')
    self.assertEquals(data['value'], 'value-01')
    self.assertRaises(RaleighException, sset.get, 'key-01')

    data = sset.scan(2)
    self.assertEquals(data['keys'], ['key-02'])
    self.assertEquals(data['values'], ['value-02'])

    data = sset.update('key-02', 'value-02b')
    self.assertEquals(data['old_value'], 'value-02')
    data = sset.get('key-02')
    self.assertEquals(data['value'], 'value-02b')

  def test_scanner(self):
    oid = self.createObject(RaleighSSet.TYPE)
    sset = RaleighSSet(self.client, oid)

    for i in xrange(10):
      sset.insert('key-%2d' % i, 'value-%2d' % i)

    for i, (key, value) in enumerate(sset.create_scanner()):
      self.assertEquals(key, 'key-%2d' % i)
      self.assertEquals(value, 'value-%2d' % i)

  def test_txn_key_locked(self):
    oid = self.createObject(RaleighSSet.TYPE)
    sset = RaleighSSet(self.client, oid)

    txn_1 = RaleighTransaction(self.client)
    txn_1.begin()

    txn_2 = RaleighTransaction(self.client)
    txn_2.begin()

    sset.insert('key-01', 'value-01', txn_1.txn_id)
    self.assertRaises(RaleighException, sset.insert, 'key-01', 'value-02', txn_2.txn_id)

    txn_2.rollback()
    txn_1.commit()

    data = sset.get('key-01')
    self.assertEquals(data['value'], 'value-01')

  def test_txn_multi_obj(self):
      def _sset_get(sset, key, txn_id, expected):
        try:
          data = sset.get(key, txn_id)
          print sset, data
          assert data['value'] == expected, (data['value'] == expected)
        except Exception as e:
          print sset, e
          assert expected == None, expected

      oid_1 = self.createObject(RaleighSSet.TYPE)
      sset_1 = RaleighSSet(self.client, oid_1)

      oid_2 = self.createObject(RaleighSSet.TYPE)
      sset_2 = RaleighSSet(self.client, oid_2)

      key = 'key-01'
      value = 'value-01'

      # Test Rollback
      txn = RaleighTransaction(self.client)
      txn.begin()
      sset_1.insert(key, value, txn.txn_id)
      sset_2.insert(key, value, txn.txn_id)

      self.assertRaises(RaleighException, sset_1.get, key)
      self.assertRaises(RaleighException, sset_2.get, key)
      data = sset_1.get(key, txn.txn_id)
      self.assertEquals(data['value'], value)
      data = sset_2.get(key, txn.txn_id)
      self.assertEquals(data['value'], value)

      txn.rollback()

      self.assertRaises(RaleighException, sset_1.get, key)
      self.assertRaises(RaleighException, sset_2.get, key)

      # Test Commit
      txn = RaleighTransaction(self.client)
      txn.begin()
      sset_1.insert(key, value, txn.txn_id)
      sset_2.insert(key, value, txn.txn_id)
      self.assertRaises(RaleighException, sset_1.get, key)
      self.assertRaises(RaleighException, sset_2.get, key)
      data = sset_1.get(key, txn.txn_id)
      self.assertEquals(data['value'], value)
      data = sset_2.get(key, txn.txn_id)
      self.assertEquals(data['value'], value)
      txn.commit()

      data = sset_1.get(key)
      self.assertEquals(data['value'], value)
      data = sset_2.get(key)
      self.assertEquals(data['value'], value)

  def test_long_run(self):
    oid = self.createObject(RaleighSSet.TYPE)
    sset = RaleighSSet(self.client, oid)

    hkey = lambda x: sha1('%d' % x).hexdigest()
    hval = lambda x, v: sha1('%d-v%s' % (x, v)).hexdigest()
    NKEYS = (64 * 1024) / (20 + 40)
    VERIFY_MOD = 100

    class Data:
      def __init__(self):
        self.data = []
        self.rm_keys = set()

      def __len__(self):
        return len(self.data)

      def get(self, key):
        index = self._get_index(key)
        return index, self.data[index][1]

      def insert(self, key, value):
        insort(self.data, (key, value))

      def update(self, key, value):
        index = self._get_index(key)
        self.data[index] = (key, value)

      def remove(self, key):
        del self.data[self._get_index(key)]
        self.rm_keys.add(key)

      def is_removed(self, key):
        return key in self.rm_keys

      def _get_index(self, key):
        index = bisect_left(self.data, (key, None))
        if index != len(self.data) and self.data[index][0] == key:
          return index
        raise ValueError

    def _verify_sset_data(sset, test_data):
      for j in xrange(len(test_data)):
        key = hkey(j)
        if test_data.is_removed(key):
          self.assertRaises(RaleighException, sset.get, key)
        else:
          _, value = test_data.get(key)
          result = sset.get(key)
          self.assertEquals(result['value'], value)

      for j, (key, value) in enumerate(sset.create_scanner()):
        index, test_value = test_data.get(key)
        self.assertEquals(test_value, value)
        self.assertEquals(index, j)

    test_data = Data()

    # Insert and Verify
    for i in xrange(NKEYS):
      key, value = hkey(i), hval(i, 1)
      sset.insert(key, value)
      test_data.insert(key, value)
      if i % VERIFY_MOD == 0:
        _verify_sset_data(sset, test_data)
    _verify_sset_data(sset, test_data)

    # Update and Verify
    for i in xrange(0, NKEYS, 2):
      key, value = hkey(i), hval(i, 2)
      sset.update(key, value)
      test_data.update(key, value)
      if i % VERIFY_MOD == 0:
        _verify_sset_data(sset, test_data)
    _verify_sset_data(sset, test_data)

    # Delete and Verify
    for i in xrange(1, NKEYS, 2):
      key = hkey(i)
      sset.pop(key)
      test_data.remove(key)
      if i % VERIFY_MOD == 0:
        _verify_sset_data(sset, test_data)
    _verify_sset_data(sset, test_data)

if __name__ == '__main__':
  import unittest
  unittest.main()
