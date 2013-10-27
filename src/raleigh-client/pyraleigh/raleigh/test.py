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

from uuid import uuid1

import unittest

from raleigh.objects import RaleighClient
from iopoll import IOPoll

class RaleighTestCase(unittest.TestCase):
  RALEIGH_HOST = '127.0.0.1'
  RALEIGH_PORT = 11215
  REMOVE_OBJECTS = True

  def setUp(self):
    self.iopoll = IOPoll()
    self.iopoll.open()
    self.client = RaleighClient()
    self.client.REPLY_MAX_WAIT = 3
    self.client.connect(self.RALEIGH_HOST, self.RALEIGH_PORT)
    self.iopoll.add(self.client)
    self.objects = {}

  def tearDown(self):
    if self.REMOVE_OBJECTS:
      for oid in self.objects:
        self.removeObject(oid)
    self.iopoll.remove(self.client)
    self.client.close()
    self.iopoll.close()

  def generateName(self):
    return str(uuid1())

  def createObject(self, object_type):
    name = self.generateName()
    data = self.client.semantic_create(name, object_type)
    oid = data['oid']
    self.objects[oid] = name
    return oid

  def removeObject(self, oid):
    name = self.objects[oid]
    self.client.semantic_delete(name)
