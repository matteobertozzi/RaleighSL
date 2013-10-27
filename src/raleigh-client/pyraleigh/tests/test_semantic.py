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

from raleigh.objects import RaleighCounter
from raleigh.client import RaleighException
from raleigh.test import RaleighTestCase

class TestSemantic(RaleighTestCase):
  def test_simple(self):
    name_1 = self.generateName()
    name_2 = self.generateName()

    data = self.client.semantic_create(name_1, RaleighCounter.TYPE)
    oid = data['oid']

    data = self.client.semantic_open(name_1)
    self.assertEquals(data['oid'], oid)
    self.assertRaises(RaleighException, self.client.semantic_open, name_2)

    self.client.semantic_rename(name_1, name_2)
    self.assertRaises(RaleighException, self.client.semantic_open, name_1)
    data = self.client.semantic_open(name_2)
    self.assertEquals(data['oid'], oid)

    self.assertRaises(RaleighException, self.client.semantic_delete, name_1)
    data = self.client.semantic_delete(name_2)
    print data
    self.assertRaises(RaleighException, self.client.semantic_open, name_1)
    self.assertRaises(RaleighException, self.client.semantic_open, name_2)

if __name__ == '__main__':
  import unittest
  unittest.main()
