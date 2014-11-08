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

from zcl.rpc_coding import FieldReader, FieldWriter
from zcl.ipc_client import IpcMsgClient
from zcl.util import Histogram, humanSize, humanTime
from time import time, sleep

metrics_rpc_histogram_fields = {
  1: ('bounds', 'list[uint]', None),
  2: ('events', 'list[uint]', None),
}

metrics_rpc_iopoll_load_fields = {
  1: ('max_events', 'uint', None),
  2: ('events', 'list[uint]', None),
  3: ('idle',   'list[uint]', None),
  4: ('active', 'list[uint]', None),
}

metrics_rpc_info_fields = {
  1: ('uptime', 'uint', None),
  3: ('core_count', 'uint', None),
}

metrics_rpc_rusage_fields = {
  1: ('utime',   'uint', None),
  2: ('stime',   'uint', None),
  3: ('maxrss',  'uint', None),
  4: ('minflt',  'uint', None),
  5: ('majflt',  'uint', None),
  6: ('inblock', 'uint', None),
  7: ('oublock', 'uint', None),
  8: ('nvcsw',   'uint', None),
  9: ('nivcsw',  'uint', None),
}

metrics_rpc_memory_fields = {
  1: ('pool_used', 'uint', None),
  2: ('pool_alloc', 'uint', None),
  3: ('extern_used', 'uint', None),
  4: ('extern_alloc', 'uint', None),
  5: ('histogram', metrics_rpc_histogram_fields, None),
}

metrics_rpc_iopoll_fields = {
  1: ('io_load',  metrics_rpc_iopoll_load_fields, None),
  2: ('io_wait',  metrics_rpc_histogram_fields, None),
  3: ('io_read',  metrics_rpc_histogram_fields, None),
  4: ('io_write', metrics_rpc_histogram_fields, None),
  5: ('event',    metrics_rpc_histogram_fields, None),
  6: ('timer',    metrics_rpc_histogram_fields, None),
}

class MetricsClient(IpcMsgClient):
  def info(self):
    writer = FieldWriter()
    return self._sync_send_recv(1, metrics_rpc_info_fields, writer)

  def rusage(self):
    writer = FieldWriter()
    return self._sync_send_recv(2, metrics_rpc_rusage_fields, writer)

  def memory(self, core):
    writer = FieldWriter()
    writer.write_uint(1, core)
    return self._sync_send_recv(3, metrics_rpc_memory_fields, writer)

  def iopoll(self, core):
    writer = FieldWriter()
    writer.write_uint(1, core)
    return self._sync_send_recv(4, metrics_rpc_iopoll_fields, writer)

  def _sync_send_recv(self, msg_type, fields, writer):
    self.send_message(msg_type, writer.get_body(), writer.get_blob())
    _, resp_msg_type, resp_msg_id, data = self._sync_recv(fields)
    assert msg_type == resp_msg_type, (msg_type, resp_msg_type)
    return data

if __name__ == '__main__':
  client = MetricsClient()
  with client.connection(None, 'localhost', 11213):
    print client.rusage()
    info = client.info()
    print info
    for i in xrange(info.get('core_count')):
      memdata = client.memory(i)
      print memdata
      Histogram.from_json('memory', memdata['histogram']).dump(humanSize)

      iodata = client.iopoll(i)
      print iodata
      Histogram.from_json('io_wait', iodata.get('io_wait')).dump(humanTime)
      Histogram.from_json('io_read', iodata.get('io_read')).dump(humanTime)
      Histogram.from_json('io_write', iodata.get('io_write')).dump(humanTime)
      Histogram.from_json('event', iodata.get('event')).dump(humanTime)
      Histogram.from_json('timer', iodata.get('timer')).dump(humanTime)
