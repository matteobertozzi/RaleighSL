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

from zcl.ipc_client import IpcMsgClient
from time import time, sleep

if __name__ == '__main__':
  ECHO_HOST = 'localhost'
  ECHO_PORT = 11213

  client = IpcMsgClient()
  with client.connection(None, ECHO_HOST, ECHO_PORT):
    """
    NREQUESTS = 200
    DATA = 'foo'
    st = time()
    for i in xrange(NREQUESTS):
      client.send_message(i, DATA)
      client._sync_recv({})
    total_time = time() - st
    print '%.3fsec - %.3fmsg/sec' % (total_time, (NREQUESTS) / total_time)
    """

    client.send_message(1, '', '')
    print client._sync_recv({})

    client.send_message(2, 'a', '')
    print client._sync_recv({})

    client.send_message(3, '', 'b')
    print client._sync_recv({})

    client.send_message(4, 'a', 'b')
    print client._sync_recv({})

    st = time()
    for _ in xrange(1000):
      for i in xrange(32):
        data = 'z' * (1 + i)
        client.send_message(i, '', data)
        print client._sync_recv({})

      for i in xrange(32, 0, -1):
        data = 'z' * (1 + i)
        client.send_message(i, '', data)
        print data
        print client._sync_recv({})
    sec = time() - st
    print '%.3freq/sec' % ((64 * 1000) / sec)