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

import sys

from time import time, sleep

from raleighfs import RaleighFS
from netclient import humanSize
from iopoll import IOPoll
import coding

def dump_stat(total_bytes, time):
  total_mib = total_bytes / (1024 * 1024.0)
  sys.stdout.write('\r%s %s/sec' % (humanSize(total_bytes), humanSize(total_bytes / time)))
  sys.stdout.flush()


def test(client):
  if 0:
    for j in xrange(10):
      info = []
      buf = []
      for i in xrange(j + 1):
        x = str(coding.z_encode_field(i, i + 1)) + 'x' * (i + 1)
        buf.append(x)
        info.append((i, len(x)))
      client.send(''.join(buf))
      print 'send %d %r from %d to %d' % (len(''.join(buf)), info, 0, j)
      sleep(0.15)
  else:
    for i in xrange(700):
      data = 'x' * (1 + (i % 300))
      head = str(coding.z_encode_field(i, len(data)))
      client.send(head + data)
      print 'send head=%d data=%d field=%d -> buf %d' % (len(head), len(data), i, len(head) + len(data))
      if i % 7 == 0: sleep(0.1)
      #sleep(0.05)
  sleep(10)

def test_cross_tag(client):
  buf = str(coding.z_encode_field(12, 13)) + 'x' * (13)
  buf += str(coding.z_encode_field(27, 28)) + 'y' * (28)
  client.send(''.join(buf))

def encodeVarint(value):
  data = bytearray()
  bits = value & 0x7f
  value >>= 7
  while value != 0:
    data.append(0x80 | bits)
    bits = value & 0x7f
    value >>= 7
  data.append(bits)
  return data

if __name__ == '__main__':
  iopoll = IOPoll()
  iopoll.start()
  try:
    client = RaleighFS()
    with client.connection('127.0.0.1', 11215):
      iopoll.add(client)
      for i in xrange(16):
        client.send('x' * (i + 1))
        count = 0
        while count < 1:
          for data in client.recv():
            print 'data=', len(data), data
            count += 1
      for i in xrange(10):
        sleep(5)
  except KeyboardInterrupt:
    pass
  finally:
    iopoll.stop()
