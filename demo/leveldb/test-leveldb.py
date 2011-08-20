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

import random
import socket
import time

class LevelDBClient(object):
    def open(self, host, port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))

    def close(self):
        self.sock.close()

    def put(self, key, value):
        self._send('set %s %d\r\n' % (key, len(value)), value, '\n')
        return self.sock.recv(8192).strip() == "STORED"

    def get(self, key):
        self._send('get %s\r\n' % (key,))

        rbuf = []
        while 1:
            data = self.sock.recv(8192)
            rbuf.append(data)
            if data.endswith('END\r\n'):
                break

        def _extractKV(data):
            index = data.index(' ')
            key = data[:index]
            index2 = data.index('\n', index)
            size = int(data[index:index2])
            return data[index2+1:index2+1+size]

        rbuf = ''.join(rbuf)[:-5]
        return [_extractKV(d) for d in rbuf.split('VALUE ')[1:]]

    def delete(self, key):
        self._send("delete %s\r\n" % (key,))
        return self.sock.recv(8192).strip()

    def clear(self):
        self._send('flush_all\r\n')
        return self.sock.recv(8192).strip() == "OK"

    def stats(self):
        self._send('stats\r\n')

        rbuf = []
        while 1:
            data = self.sock.recv(8192)
            rbuf.append(data)
            if data.endswith('\r\nEND\r\n'):
                break
        return ''.join(data)[:-7]

    def _send(self, *args):
        buf = ''.join(args)
        #print "'%s'" % buf.strip()
        return self.sock.send(buf)

def randstr(n):
    alphabet = [chr(i) for i in xrange(ord('a'), ord('z') + 1)]

    def _rand():
        x = random.choice(alphabet)
        if random.choice([0, 1, 2, 3, 5, 6, 7, 10]) % 2 == 0:
            x = x.upper()
        return x

    result = [_rand() for i in xrange(n)]
    return ''.join(result)

if __name__ == '__main__':
    leveldb = LevelDBClient()
    leveldb.open('127.0.0.1', 11213)

    table = [(randstr(20), randstr(40)) for i in xrange(10000)]
    qps = lambda q, s: q / s
    kps = lambda b, s: (b / 1024.0) / s


    __st = time.time()
    for key, value in table:
        leveldb.put(key, value)
    __et = time.time() - __st
    print '[T] SET %.3f query/sec (%.3fKiB/sec)' % \
            (qps(len(table), __et), kps(len(table) * 60.0, __et))

    __st = time.time()
    for key, value in table:
        values = leveldb.get(key)
        assert values[0] == value
        leveldb.get(key)
    __et = time.time() - __st
    print '[T] GET %.3f query/sec (%.3fKiB/sec)' % \
            (qps(len(table), __et), kps(len(table) * 80.0, __et))

    __st = time.time()
    for key, _ in table:
        leveldb.delete(key)
    __et = time.time() - __st
    print '[T] DEL %.3f query/sec (%.3fKiB/sec)' % \
            (qps(len(table), __et), kps(len(table) * 20.0,  __et))

    leveldb.clear()

    leveldb.close()

