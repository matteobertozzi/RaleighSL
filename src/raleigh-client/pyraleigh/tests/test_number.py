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

from raleigh.objects import RaleighNumber
from raleigh.objects import RaleighTransaction
from raleigh.client import RaleighException
from raleigh.test import RaleighTestCase

from random import randint

class TestNumber(RaleighTestCase):
  def test_set(self):
    oid = self.createObject(RaleighNumber.TYPE)
    number = RaleighNumber(self.client, oid)
    for _ in xrange(10):
      value = randint(0, 1000)
      number.set(value)
      data = number.get()
      self.assertEquals(data['value'], value)

  def test_cas(self):
    oid = self.createObject(RaleighNumber.TYPE)
    number = RaleighNumber(self.client, oid)

    data = number.cas(0, 50)
    self.assertEquals(data['value'], 0)
    data = number.get()
    self.assertEquals(data['value'], 50)

    self.assertRaises(RaleighException, number.cas, 10, 100)

    data = number.cas(50, -100)
    self.assertEquals(data['value'], 50)
    data = number.get()
    self.assertEquals(data['value'], -100)

  def test_inc(self):
    oid = self.createObject(RaleighNumber.TYPE)
    number = RaleighNumber(self.client, oid)

    data = number.get()
    self.assertEquals(data['value'], 0)
    for value in xrange(1, 11):
      data = number.inc()
      self.assertEquals(data['value'], value)
    for i in xrange(1, 15):
      data = number.dec()
      self.assertEquals(data['value'], 10 - i)

  def test_add(self):
    oid = self.createObject(RaleighNumber.TYPE)
    number = RaleighNumber(self.client, oid)

    data = number.add(5)
    self.assertEquals(data['value'], 5)

    data = number.add(-10)
    self.assertEquals(data['value'], -5)

  def test_mul(self):
    oid = self.createObject(RaleighNumber.TYPE)
    number = RaleighNumber(self.client, oid)

    number.set(5)
    data = number.get()
    self.assertEquals(data['value'], 5)

    data = number.mul(3)
    self.assertEquals(data['value'], 15)

    data = number.mul(-10)
    self.assertEquals(data['value'], -150)

  def test_div(self):
    oid = self.createObject(RaleighNumber.TYPE)
    number = RaleighNumber(self.client, oid)

    number.set(10)
    data = number.get()
    self.assertEquals(data['value'], 10)

    data = number.divmod(2)
    self.assertEquals(data['mod'], 0)
    self.assertEquals(data['value'], 5)

    data = number.divmod(3)
    self.assertEquals(data['mod'], 2)
    self.assertEquals(data['value'], 1)

    self.assertRaises(RaleighException, number.divmod, 0)

    data = number.divmod(7)
    self.assertEquals(data['mod'], 1)
    self.assertEquals(data['value'], 0)

  def test_txn(self):
    oid = self.createObject(RaleighNumber.TYPE)
    number = RaleighNumber(self.client, oid)

    txn_1 = RaleighTransaction(self.client)
    txn_1.begin()

    txn_2 = RaleighTransaction(self.client)
    txn_2.begin()

    data = number.inc(txn_1.txn_id)
    self.assertEquals(data['value'], 1)
    self.assertRaises(RaleighException, number.set, 10, txn_2.txn_id)
    self.assertRaises(RaleighException, number.cas, 10, 20, txn_2.txn_id)
    self.assertRaises(RaleighException, number.add, 10, txn_2.txn_id)
    self.assertRaises(RaleighException, number.mul, 10, txn_2.txn_id)
    data = number.get()
    self.assertEquals(data['value'], 0)
    data = number.get(txn_1.txn_id)
    self.assertEquals(data['value'], 1)

    txn_1.commit()

    number.inc(txn_2.txn_id)
    self.assertRaises(RaleighException, number.set, 10)
    self.assertRaises(RaleighException, number.cas, 10, 20)
    self.assertRaises(RaleighException, number.add, 10)
    self.assertRaises(RaleighException, number.mul, 10)
    data = number.get()
    self.assertEquals(data['value'], 1)
    data = number.get(txn_2.txn_id)
    self.assertEquals(data['value'], 2)

    txn_2.rollback()

    data = number.inc()
    self.assertEquals(data['value'], 2)

if __name__ == '__main__':
  import unittest
  unittest.main()