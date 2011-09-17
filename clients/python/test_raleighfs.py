#!/usr/bin/env python
#
#   Copyright 2011 Matteo Bertozzi
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

import unittest
import sys

import raleighfs

RALEIGHFS_HOST = "127.0.0.1"
RALEIGHFS_PORT = 11215

class _RaleighFSTestCase(unittest.TestCase):
    def setUp(self):
        self.fs = raleighfs.RaleighFS()
        self.fs.connect(RALEIGHFS_HOST, RALEIGHFS_PORT)
        self.assertTrue(self.fs.ping())

    def tearDown(self):
        self.assertTrue(self.fs.ping())
        self.fs.disconnect()
        self.fs = None

class _RaleighFSObjectTestCase(_RaleighFSTestCase):
    OBJECT_TYPE = None
    OBJECT_NAME = None

    def setUp(self):
        super(_RaleighFSObjectTestCase, self).setUp()
        self.fsobject = self.fs.create(self.OBJECT_TYPE, self.OBJECT_NAME)
        self.assertTrue(self.fs.exists(self.OBJECT_NAME))

    def tearDown(self):
        self.assertTrue(self.fs.unlink(self.fsobject))
        self.assertFalse(self.fs.exists(self.OBJECT_NAME))
        super(_RaleighFSObjectTestCase, self).tearDown()

    def testExists(self):
        self.fs.exists(self.OBJECT_NAME)

    def testRename(self):
        new_name = self.OBJECT_NAME + '.renamed'

        self.assertTrue(self.fs.rename(self.fsobject, new_name))
        self.assertEquals(self.fsobject.name, new_name)
        self.assertFalse(self.fs.exists(self.OBJECT_NAME))
        self.assertTrue(self.fs.exists(new_name))

        self.assertTrue(self.fs.rename(self.fsobject, self.OBJECT_NAME))
        self.assertEquals(self.fsobject.name, self.OBJECT_NAME)
        self.assertTrue(self.fs.exists(self.OBJECT_NAME))
        self.assertFalse(self.fs.exists(new_name))

class TestKeyValue(_RaleighFSObjectTestCase):
    OBJECT_TYPE = raleighfs.Kv
    OBJECT_NAME = '/test/kv'

    def testSet(self):
        for i in range(20):
            key = 'key-%02d' % i
            value = 'Value %d' % i
            self.assertTrue(self.fsobject.set(key, value))
            self.assertEquals(self.fsobject.get([key]), {key:(['0'], value)})

    def testCas(self):
        key = 'keyX'
        self.assertTrue(self.fsobject.set(key, 'V0'))
        self.assertEquals(self.fsobject.get([key]), {key:(['0'], 'V0')})

        self.assertFalse(self.fsobject.cas(key, 'V1', 1))
        self.assertEquals(self.fsobject.get([key]), {key:(['0'], 'V0')})

        self.assertTrue(self.fsobject.cas(key, 'V1', 0))
        self.assertEquals(self.fsobject.get([key]), {key:(['1'], 'V1')})

        self.assertFalse(self.fsobject.cas(key, 'V2', 0))
        self.assertEquals(self.fsobject.get([key]), {key:(['1'], 'V1')})

        self.assertTrue(self.fsobject.cas(key, 'V2', 1))
        self.assertEquals(self.fsobject.get([key]), {key:(['2'], 'V2')})

    def testClear(self):
        for i in range(20):
            self.assertTrue(self.fsobject.set('key-%02d' % i, 'Value %d' % i))

        for i in range(20):
            self.assertEquals(self.fsobject.get('key-%02d' % i), {})

    def testRemove(self):
        for i in range(20):
            self.assertTrue(self.fsobject.set('key-%02d' % i, 'Value %d' % i))

        for i in range(0, 20, 2):
            self.assertTrue(self.fsobject.remove('key-%02d' % i))

        for i in range(1, 20, 2):
            key = 'key-%02d' % i
            self.assertEquals(self.fsobject.get([key]), {key: (['0'], 'Value %d' % i)})

        for i in range(0, 20, 2):
            self.assertEquals(self.fsobject.get(['key-%02d' % i]), {})

    def testModifyData(self):
        key = 'KeyX'
        self.assertTrue(self.fsobject.set(key, 'Hello'))
        self.assertEquals(self.fsobject.get([key]), {key:(['0'], 'Hello')})

        self.assertTrue(self.fsobject.append(key, ' World'))
        self.assertEquals(self.fsobject.get([key]), {key:(['1'], 'Hello World')})

        self.assertTrue(self.fsobject.prepend(key, 'This is a '))
        self.assertEquals(self.fsobject.get([key]), {key:(['2'], 'This is a Hello World')})

        self.assertTrue(self.fsobject.append(key, ' Test!'))
        self.assertEquals(self.fsobject.get([key]), {key:(['3'], 'This is a Hello World Test!')})

class TestSSet(_RaleighFSObjectTestCase):
    OBJECT_TYPE = raleighfs.SSet
    OBJECT_NAME = '/tests/sset'

    def testInsert(self):
        keys = []
        for i in xrange(20):
            key = 'key-%02d' % i
            value = 'value %02d' % i
            self.assertTrue(self.fsobject.insert(key, value))
            self.assertEquals(self.fsobject.get([key]), {key:value})
            self.assertEquals(self.fsobject.length(), i + 1)
            keys.append(key)
        self.assertEquals(self.fsobject.keys(), keys)

    def testUpdate(self):
        for i in xrange(20):
            key = 'key-%02d' % i
            value = 'value %d' % i
            self.assertTrue(self.fsobject.insert(key, value))
            self.assertEquals(self.fsobject.get([key]), {key:value})

        self.assertEquals(self.fsobject.length(), 20)
        for i in xrange(20):
            key = 'key-%02d' % i
            value1 = 'value %d' % i
            value2 = 'value2 %d' % i
            self.assertEquals(self.fsobject.get([key]), {key:value1})
            self.assertTrue(self.fsobject.update(key, value2))
            self.assertEquals(self.fsobject.get([key]), {key:value2})
            self.assertEquals(self.fsobject.length(), 20)

    def testClear(self):
        for i in xrange(20):
            self.assertTrue(self.fsobject.insert('K%d' % i, 'V%d' % i))
        self.assertEquals(self.fsobject.length(), 20)
        self.assertTrue(self.fsobject.clear())
        self.assertEquals(self.fsobject.length(), 0)
        self.assertEquals(self.fsobject.keys(), [])

    def testGetFirst(self):
        for i in xrange(9, 0, -1):
            key = 'key-%02d' % i
            value = 'value %02d' % i
            self.assertTrue(self.fsobject.insert(key, value))
            self.assertEquals(self.fsobject.get([key]), {key:value})
            self.assertEquals(self.fsobject.getFirst(), {key:value})
            self.assertEquals(self.fsobject.getLast(), {'key-09':'value 09'})

        self.assertEquals(self.fsobject.getIndex(0, 4), dict(('key-%02d' % i, 'value %02d' % i) for i in range(1, 5)))
        self.assertEquals(self.fsobject.getIndex(3, 6), dict(('key-%02d' % i, 'value %02d' % i) for i in range(4, 10)))

        self.assertEquals(self.fsobject.getRange('key-01', 'key-04'), dict(('key-%02d' % i, 'value %02d' % i) for i in range(1, 5)))
        self.assertEquals(self.fsobject.getRange('key-04', 'key-09'), dict(('key-%02d' % i, 'value %02d' % i) for i in range(4, 10)))

    def testRemove(self):
        for i in xrange(20):
            self.assertTrue(self.fsobject.insert('key-%02d' % i, 'value %02d' % i))

        for i in xrange(20):
            key = 'key-%02d' % i
            self.assertEquals(self.fsobject.get([key]), {key:'value %02d' % i})
            self.assertTrue(self.fsobject.remove([key]))
            self.assertEquals(self.fsobject.get([key]), {})
            self.assertEquals(self.fsobject.length(), 19 - i)

    def testRemoveRange(self):
        for i in xrange(20):
            self.assertTrue(self.fsobject.insert('key-%02d' % i, 'value %02d' % i))

        self.assertEquals(self.fsobject.length(), 20)
        self.assertTrue(self.fsobject.removeRange('key-00', 'key-10'))
        self.assertEquals(self.fsobject.length(), 9)
        keys = self.fsobject.getIndex(0, 20).keys()
        self.assertEquals(len(keys), 9)
        self.assertEquals(set(keys), set('key-%02d' % i for i in range(11, 20)))
        self.assertEquals(set(self.fsobject.keys()), set(keys))

        self.fsobject.removeRange('key-15', 'key-25')
        keys = self.fsobject.getIndex(0, 20).keys()
        self.assertEquals(len(keys), 4)
        self.assertEquals(set(keys), set('key-%02d' % i for i in range(11, 15)))
        self.assertEquals(set(self.fsobject.keys()), set(keys))

    def testRemoveIndex(self):
        for i in xrange(20):
            self.assertTrue(self.fsobject.insert('key-%02d' % i, 'value %02d' % i))

        self.fsobject.removeIndex(0, 10)
        keys = self.fsobject.getIndex(0, 20).keys()
        self.assertEquals(len(keys), 10)
        self.assertEquals(set(keys), set('key-%02d' % i for i in range(10, 20)))

        self.fsobject.removeIndex(5, 4)
        keys = self.fsobject.getIndex(0, 20).keys()
        self.assertEquals(len(keys), 6)
        self.assertEquals(set(keys), set(['key-10', 'key-11', 'key-12', 'key-13', 'key-14', 'key-19']))

class TestDeque(_RaleighFSObjectTestCase):
    OBJECT_TYPE = raleighfs.Deque
    OBJECT_NAME = '/tests/deque'

    def testPush(self):
        first_data = 'test-data-first'
        self.assertTrue(self.fsobject.push(first_data))
        self.assertEquals(self.fsobject.get(), first_data)
        self.assertEquals(self.fsobject.getFront(), first_data)
        self.assertEquals(self.fsobject.getBack(), first_data)

        for i in xrange(20):
            data = 'test-data-%d' % i
            self.assertTrue(self.fsobject.push(data))
            self.assertEquals(self.fsobject.get(), first_data)
            self.assertEquals(self.fsobject.getFront(), first_data)
            self.assertEquals(self.fsobject.getBack(), data)

    def testBack(self):
        first_data = 'test-data-first'
        self.assertTrue(self.fsobject.pushBack(first_data))
        self.assertEquals(self.fsobject.get(), first_data)
        self.assertEquals(self.fsobject.getFront(), first_data)
        self.assertEquals(self.fsobject.getBack(), first_data)

        for i in xrange(20):
            data = 'test-data-%d' % i
            self.assertTrue(self.fsobject.pushBack(data))
            self.assertEquals(self.fsobject.get(), first_data)
            self.assertEquals(self.fsobject.getFront(), first_data)
            self.assertEquals(self.fsobject.getBack(), data)

    def testFront(self):
        first_data = 'test-data-first'
        self.assertTrue(self.fsobject.pushFront(first_data))
        self.assertEquals(self.fsobject.get(), first_data)
        self.assertEquals(self.fsobject.getFront(), first_data)
        self.assertEquals(self.fsobject.getBack(), first_data)

        for i in xrange(20):
            data = 'test-data-%d' % i
            self.assertTrue(self.fsobject.pushFront(data))
            self.assertEquals(self.fsobject.get(), data)
            self.assertEquals(self.fsobject.getFront(), data)
            self.assertEquals(self.fsobject.getBack(), first_data)

    def testPop(self):
        for i in xrange(20):
            self.assertTrue(self.fsobject.push('test-data-%d' % i))
        self.assertEquals(self.fsobject.length(), 20)

        for i in xrange(20):
            self.assertEquals(self.fsobject.pop(), 'test-data-%d' % i)
        self.assertEquals(self.fsobject.length(), 0)
        self.assertEquals(self.fsobject.pop(), None)
        self.assertEquals(self.fsobject.get(), None)

    def testPopFront(self):
        for i in xrange(20):
            self.assertTrue(self.fsobject.push('test-data-%d' % i))
        self.assertEquals(self.fsobject.length(), 20)

        for i in xrange(20):
            self.assertEquals(self.fsobject.popFront(), 'test-data-%d' % i)
        self.assertEquals(self.fsobject.length(), 0)
        self.assertEquals(self.fsobject.popFront(), None)
        self.assertEquals(self.fsobject.getFront(), None)

    def testPopBack(self):
        for i in xrange(20):
            self.assertTrue(self.fsobject.push('test-data-%d' % i))
        self.assertEquals(self.fsobject.length(), 20)

        for i in xrange(20):
            self.assertEquals(self.fsobject.popBack(), 'test-data-%d' % (20 - i - 1))
        self.assertEquals(self.fsobject.length(), 0)
        self.assertEquals(self.fsobject.popBack(), None)
        self.assertEquals(self.fsobject.getBack(), None)

    def testLength(self):
        self.assertEquals(self.fsobject.length(), 0)

        n = 0
        for i in xrange(20):
            n += 1
            self.assertTrue(self.fsobject.push('data-%d' % i))
            self.assertEquals(self.fsobject.length(), n)

        for i in xrange(5):
            n -= 1
            self.fsobject.popFront()
            self.assertEquals(self.fsobject.length(), n)

        for i in xrange(5):
            n -= 1
            self.fsobject.popBack()
            self.assertEquals(self.fsobject.length(), n)

        for i in xrange(5):
            n -= 1
            self.fsobject.removeFront()
            self.assertEquals(self.fsobject.length(), n)

        for i in xrange(5):
            n -= 1
            self.fsobject.removeBack()
            self.assertEquals(self.fsobject.length(), n)

        self.assertTrue(self.fsobject.clear())
        self.assertEquals(self.fsobject.length(), 0)

class TestCounter(_RaleighFSObjectTestCase):
    OBJECT_TYPE = raleighfs.Counter
    OBJECT_NAME = '/tests/counter'

    def testSet(self):
        for i in xrange(10):
            n = i * 10 + 5
            # Set Value
            value, cas = self.fsobject.set(n)
            self.assertEquals(value, n)
            self.assertEquals(cas, i + 1)

            # Get Value
            value, cas = self.fsobject.get()
            self.assertEquals(value, n)
            self.assertEquals(cas, i + 1)

    def testCas(self):
        value, cas = self.fsobject.set(10)
        self.assertEquals(value, 10)
        self.assertEquals(cas, 1)

        value, cas = self.fsobject.cas(20, cas + 1)
        self.assertEquals(value, 10)
        self.assertEquals(cas, 1)

        value, cas = self.fsobject.cas(30, cas)
        self.assertEquals(value, 30)
        self.assertEquals(cas, 2)

    def testIncr(self):
        for i in xrange(10):
            # Incr by 1
            value, cas = self.fsobject.incr()
            self.assertEquals(value, i + 1)
            self.assertEquals(cas, i + 1)

            # Get Value
            value, cas = self.fsobject.get()
            self.assertEquals(value, i + 1)
            self.assertEquals(cas, i + 1)

    def testIncrValue(self):
        n = 0
        for i in xrange(10):
            n += i

            # Incr by i
            value, cas = self.fsobject.incr(i)
            self.assertEquals(value, n)
            self.assertEquals(cas, i + 1)

            # Get Value
            value, cas = self.fsobject.get()
            self.assertEquals(value, n)
            self.assertEquals(cas, i + 1)

    def testDecr(self):
        n = 99999
        value, cas = self.fsobject.set(n)
        self.assertEquals(value, n)
        self.assertEquals(cas, 1)

        for i in xrange(10):
            n -= 1

            # Incr by 1
            value, cas = self.fsobject.decr()
            self.assertEquals(value, n)
            self.assertEquals(cas, i + 2)

            # Get Value
            value, cas = self.fsobject.get()
            self.assertEquals(value, n)
            self.assertEquals(cas, i + 2)

    def testDecrValue(self):
        n = 99999
        value, cas = self.fsobject.set(n)
        self.assertEquals(value, n)
        self.assertEquals(cas, 1)

        for i in xrange(10):
            n -= i

            # Incr by i
            value, cas = self.fsobject.decr(i)
            self.assertEquals(value, n)
            self.assertEquals(cas, i + 2)

            # Get Value
            value, cas = self.fsobject.get()
            self.assertEquals(value, n)
            self.assertEquals(cas, i + 2)

def load_tests(loader, tests, pattern):
    ftests = []
    for suite in tests:
        # Filter test class that starts with _
        ts = [t for t in suite if not t.__class__.__name__.startswith('_')]
        if len(ts) > 0:
            ftests.append(loader.suiteClass(ts))
    return loader.suiteClass(ftests)

if __name__ == '__main__':
    if sys.version_info < (2, 7):
        def _loadTestsFromTestCase(self, testCaseClass):
            """Return a suite of all tests cases contained in testCaseClass"""
            if issubclass(testCaseClass, unittest.TestSuite):
                raise TypeError("Test cases should not be derived from TestSuite." \
                                " Maybe you meant to derive from TestCase?")

            if testCaseClass.__name__.startswith('_'):
                testCaseNames = []
            else:
                testCaseNames = self.getTestCaseNames(testCaseClass)
            if not testCaseNames and hasattr(testCaseClass, 'runTest'):
                testCaseNames = ['runTest']
            loaded_suite = self.suiteClass(map(testCaseClass, testCaseNames))
            return loaded_suite
        unittest.TestLoader.loadTestsFromTestCase = _loadTestsFromTestCase

    unittest.main()

