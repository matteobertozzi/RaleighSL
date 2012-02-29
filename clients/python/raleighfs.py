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

from contextlib import contextmanager
import socket

def _trim_split(data, separator=None):
    if separator is None:
        return data.split()
    return data.split(separator)

class RaleighFS(object):
    def __init__(self):
        self._sock = None

    def connect(self, host, port):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.connect((host, port))

    def disconnect(self):
        self._sock.close()
        self._sock = None

    @contextmanager
    def connection(self, host, port):
        self.connect(host, port)
        try:
            yield
        finally:
            self.disconnect()

    def create(self, object_type, object_name):
        data = 'semantic create %s %s\r\n' % (object_name, object_type.__name__.lower())
        self.sendAndCheckOkResponse(data)
        assert issubclass(object_type, _AbstractObject)
        return object_type(self, object_name)

    def open(self, object_name):
        object_type = None # Response Lookup Type
        return object_type(self, object_name)

    def unlink(self, obj):
        assert isinstance(obj, _AbstractObject)
        data = 'semantic unlink %s\n' % (obj.name)
        return self.sendAndCheckOkResponse(data)

    def rename(self, obj, object_new_name):
        data = 'semantic rename %s %s\n' % (obj.name, object_new_name)
        self.sendAndCheckOkResponse(data)
        obj.name = object_new_name
        return True

    def exists(self, object_name):
        data = 'semantic exists %s\n' % (object_name)
        line = self.sendSingleLineRequest(data)

        if line[0] == '-':
            raise Exception(line)

        if line == '+EXISTS':
            return True

        if line == '+NOT-FOUND':
            return False

        raise Exception(line)

    def ping(self):
        line = self.sendSingleLineRequest('server ping\n')
        return line == '+PONG'

    def send(self, data):
        return self._sock.sendall(data)

    def recv(self, length=8192):
        data = []
        while True:
            v = self._sock.recv(length)
            if not v: break
            data.append(v)
            if v.endswith('\n'): break
        return ''.join(data).rstrip()

    def sendSingleLineRequest(self, data):
        response = self.sendLineRequest(data)
        assert len(response) == 1
        return response[0]

    def sendLineRequest(self, data, nlines=1):
        self.send(data)
        response = self.recv().split('\r\n')
        if len(response) < nlines:
            response.extend(self.recv().split('\r\n'))
        return response

    def sendAndCheckOkResponse(self, data):
        response = self.sendSingleLineRequest(data)
        if not response.startswith('+OK'):
          raise Exception(response)
        return True

class _AbstractObject(object):
    def __init__(self, fs, object_name):
        self.name = object_name
        self.fs = fs

    def _sendRequest(self, data):
        self.fs.send(data)
        return self.fs.recv(8192)

    def _sendSingleLineRequest(self, data):
        return self.fs.sendSingleLineRequest(data)

    def _sendLineRequest(self, data, nlines=1):
        return self.fs.sendLineRequest(data, nlines)

    def _sendAndCheckOkResponse(self, data):
        return self.fs.sendAndCheckOkResponse(data)

class _AbstractContainer(_AbstractObject):
    def clear(self):
        raise NotImplementedError

    def length(self):
        raise NotImplementedError

    def stats(self):
        raise NotImplementedError

    def _sendGetKVRequest(self, data):
        self.fs.send(data)
        response = self.fs.recv().split('\r\n')
        if response[0][0] == '-':
            raise Exception(response[0])

        key = None
        vsize = 0
        value = ''
        extra = None
        values = {}
        while True:
            for line in response:
                if line == 'END':
                    return values

                if key is None:
                    litems = line.split()
                    vsize = int(litems[0])
                    key = litems[-1]
                    assert vsize > 0
                    extra = litems[1:-1]
                    value = ''
                else:
                    value += line

                    if vsize == len(value):
                        if extra:
                            values[key] = (extra, value)
                        else:
                            values[key] = value
                        key = None

            response = self.fs.recv().split('\r\n')
        return None

class Deque(_AbstractContainer):
    def push(self, value):
        data  = 'deque push %s %d\n%s\n' % (self.name, len(value), value)
        return self._sendAndCheckOkResponse(data)

    def pushFront(self, value):
        data  = 'deque push-front %s %d\n%s\n' % (self.name, len(value), value)
        return self._sendAndCheckOkResponse(data)

    def pushBack(self, value):
        data  = 'deque push-back %s %d\n%s\n' % (self.name, len(value), value)
        return self._sendAndCheckOkResponse(data)

    def clear(self):
        data = 'deque clear %s\n' % (self.name)
        return self._sendAndCheckOkResponse(data)

    def remove(self):
        data = 'deque rm %s\n' % (self.name)
        return self._sendAndCheckOkResponse(data)

    def removeFront(self):
        data = 'deque rm-front %s\n' % (self.name)
        return self._sendAndCheckOkResponse(data)

    def removeBack(self):
        data = 'deque rm-back %s\n' % (self.name)
        return self._sendAndCheckOkResponse(data)

    def pop(self):
        data = 'deque pop %s\n' % (self.name)
        return self._sendGetRequest(data)

    def popFront(self):
        data = 'deque pop-front %s\n' % (self.name)
        return self._sendGetRequest(data)

    def popBack(self):
        data = 'deque pop-back %s\n' % (self.name)
        return self._sendGetRequest(data)

    def get(self):
        data = 'deque get %s\n' % (self.name)
        return self._sendGetRequest(data)

    def getFront(self):
        data = 'deque get-front %s\n' % (self.name)
        return self._sendGetRequest(data)

    def getBack(self):
        data = 'deque get-back %s\n' % (self.name)
        return self._sendGetRequest(data)

    def length(self):
        data = 'deque length %s\n' % (self.name)
        return int(self._sendSingleLineRequest(data))

    def stats(self):
        data = 'deque stats %s\n' % (self.name)
        return self._sendRequest(data)

    def _sendGetRequest(self, data):
        self.fs.send(data)
        response = self.fs.recv().split('\r\n')
        if response[0][0] == '-':
            raise Exception(response[0])

        size = int(response[0])
        if size == 0:
            return None

        if len(response) > 1:
            data = response[1]
        else:
            data = self.fs.recv().split('\r\n')[0]

        if size != len(data):
            raise Exception('Corrupted data. Expected %d get %d' % (int(size), len(data)))
        return data

class Counter(_AbstractObject):
    def set(self, value):
        data = 'counter set %s %u\n' % (self.name, value)
        return self._parseResponse(self._sendRequest(data))

    def cas(self, value, cas):
        data = 'counter cas %s %u %u\n' % (self.name, value, cas)
        return self._parseResponse(self._sendRequest(data))

    def get(self):
        data = 'counter get %s\n' % self.name
        return self._parseResponse(self._sendRequest(data))

    def incr(self, value=None):
        if value is None:
            data = 'counter incr %s\n' % (self.name)
        else:
            data = 'counter incr %s %u\n' % (self.name, value)
        return self._parseResponse(self._sendRequest(data))

    def decr(self, value=None):
        if value is None:
            data = 'counter decr %s\n' % (self.name)
        else:
            data = 'counter decr %s %u\n' % (self.name, value)
        return self._parseResponse(self._sendRequest(data))

    def _parseResponse(self, line):
        if line[0] == '-':
            raise Exception(line[1:])
        return tuple(int(v) for v in _trim_split(line[1:]))

class SSet(_AbstractContainer):
    def insert(self, key, value):
        data  = 'sset insert %s %s %d\n%s\n' % (self.name, key, len(value), value)
        return self._sendAndCheckOkResponse(data)

    def update(self, key, value, cas=None):
        data  = 'sset update %s %s %d\n%s\n' % (self.name, key, len(value), value)
        return self._sendAndCheckOkResponse(data)

    def clear(self):
        data = 'sset clear %s\n' % (self.name)
        return self._sendAndCheckOkResponse(data)

    def remove(self, keys):
        data = 'sset rm %s %s\n' % (self.name, ' '.join(keys))
        return self._sendRequest(data)

    def removeFirst(self):
        data = 'sset rm-first %s\n' % (self.name)
        return self._sendRequest(data)

    def removeLast(self):
        data = 'sset rm-last %s\n' % (self.name)
        return self._sendRequest(data)

    def removeRange(self, key_start, key_end):
        data = 'sset rm-range %s %s %s\n' % (self.name, key_start, key_end)
        return self._sendRequest(data)

    def removeIndex(self, index_start, index_end):
        data = 'sset rm-index %s %u %u\n' % (self.name, index_start, index_end)
        return self._sendRequest(data)

    def get(self, keys):
        data = 'sset get %s %s\n' % (self.name, ' '.join(keys))
        return self._sendGetKVRequest(data)

    def getFirst(self):
        data = 'sset get-first %s\n' % (self.name)
        return self._sendGetKVRequest(data)

    def getLast(self):
        data = 'sset get-last %s\n' % (self.name)
        return self._sendGetKVRequest(data)

    def getRange(self, key_start, key_end):
        data = 'sset get-range %s %s %s\n' % (self.name, key_start, key_end)
        return self._sendGetKVRequest(data)

    def getIndex(self, index_start, index_end):
        data = 'sset get-index %s %d %d\n' % (self.name, index_start, index_end)
        return self._sendGetKVRequest(data)

    def keys(self):
        data = 'sset keys %s\n' % (self.name)
        return self._sendGetKeys(data)

    def length(self):
        data = 'sset length %s\n' % self.name
        return int(self._sendRequest(data))

    def stats(self):
        data = 'sset stats %s\n' % self.name
        return self._sendRequest(data)

    def _sendGetKeys(self, data):
        self.fs.send(data)
        response = self.fs.recv().split('\r\n')
        if response[0][0] == '-':
            raise Exception(response[0])

        keys = []
        count = int(response.pop(0))
        while True:
            for line in response:
                keys.append(line)

            if len(keys) >= count:
                break

            response = self.fs.recv().split('\r\n')

        return keys

class Kv(_AbstractContainer):
    def set(self, key, value):
        data = 'kv set %s %s %d\n%s\n' % (self.name, key, len(value), value)
        return self._sendAndCheckOkResponse(data)

    def cas(self, key, value, cas):
        data = 'kv cas %s %s %d %d\n%s\n' % (self.name, key, len(value), cas, value)
        try:
            return self._sendAndCheckOkResponse(data)
        except Exception:
            return False

    def get(self, keys):
        data = 'kv get %s %s\n' % (self.name, ' '.join(keys))
        return self._sendGetKVRequest(data)

    def append(self, key, value):
        data = 'kv append %s %s %d\n%s\n' % (self.name, key, len(value), value)
        return self._sendAndCheckOkResponse(data)

    def prepend(self, key, value):
        data = 'kv prepend %s %s %d\n%s\n' % (self.name, key, len(value), value)
        return self._sendAndCheckOkResponse(data)

    def remove(self, key):
        data = 'kv remove %s %s\n' % (self.name, key)
        return self._sendAndCheckOkResponse(data)

if __name__ == '__main__':
    fs = RaleighFS()
    with fs.connection('127.0.0.1', 11215):
        print 'COUNTER'
        print '=================================================='
        counter = fs.create(Counter, '/counters/test1')
        print counter.set(10)
        for i in xrange(10):
            print counter.incr()
            print counter.incr(5)
            print counter.decr()
            print counter.decr(5)

        print 'DEQUE'
        print '=================================================='
        deque = fs.create(Deque, '/deque/test1')
        for i in xrange(5):
            print deque.pushBack('Value %d' % (i + 5))
        for i in xrange(5):
            print deque.pushFront('Value %d' % (4 - i))

        print deque.getFront()
        print deque.getBack()
        print deque.get()

        for i in xrange(5):
            print deque.popFront()
            print deque.popBack()
