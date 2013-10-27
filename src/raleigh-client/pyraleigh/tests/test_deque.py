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

from raleigh.objects import RaleighDeque
from raleigh.objects import RaleighTransaction
from raleigh.client import RaleighException
from raleigh.test import RaleighTestCase

class TestDeque(RaleighTestCase):
  def test_simple(self):
    oid = self.createObject(RaleighDeque.TYPE)
    deque = RaleighDeque(self.client, oid)

    deque.push_front('A')
    deque.push_front('B')
    deque.push_back('C')
    deque.push_back('D')
    deque.push_front('E')
    deque.push_back('F')
    # E B A C D F
    data = deque.pop_front()
    self.assertEquals(data['data'], 'E')
    data = deque.pop_front()
    self.assertEquals(data['data'], 'B')
    data = deque.pop_back()
    self.assertEquals(data['data'], 'F')
    data = deque.pop_back()
    self.assertEquals(data['data'], 'D')
    data = deque.pop_front()
    self.assertEquals(data['data'], 'A')
    data = deque.pop_back()
    self.assertEquals(data['data'], 'C')
    self.assertRaises(RaleighException, deque.pop_back)
    self.assertRaises(RaleighException, deque.pop_front)

  def test_txn(self):
    oid = self.createObject(RaleighDeque.TYPE)
    deque = RaleighDeque(self.client, oid)

    txn_1 = RaleighTransaction(self.client)
    txn_1.begin()

    txn_2 = RaleighTransaction(self.client)
    txn_2.begin()

    deque.push_front('A', txn_1.txn_id)
    self.assertRaises(RaleighException, deque.push_front, 'B', txn_2.txn_id)
    self.assertRaises(RaleighException, deque.push_front, 'B')
    self.assertRaises(RaleighException, deque.pop_front, txn_2.txn_id)
    self.assertRaises(RaleighException, deque.pop_front)

    data = deque.pop_front(txn_1.txn_id)
    self.assertEquals(data['data'], 'A')
    deque.push_front('C', txn_1.txn_id)
    txn_1.commit()

    deque.push_back('D', txn_2.txn_id)
    self.assertRaises(RaleighException, deque.push_back, 'E')
    self.assertRaises(RaleighException, deque.pop_back)
    data = deque.pop_back(txn_2.txn_id)
    self.assertEquals(data['data'], 'D')
    deque.push_back('F', txn_2.txn_id)
    txn_2.rollback()

    data = deque.pop_front()
    self.assertEquals(data['data'], 'C')

if __name__ == '__main__':
  import unittest
  unittest.main()
