#!/usr/bin/env python
#
#   Copyright 2011-2014 Matteo Bertozzi
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

from websocket import WebSocketTcpServer, WebSocketHandler
from http import HttpTcpServer, HttpRequestHandler
from service import AbstractService
from r5l_metrics import R5lMetrics

import time
import os
import re

class MonitorHttpRequestHandler(HttpRequestHandler):
  RESOURCES_DIR = 'res'
  PAGES_DIR = '.'

  @HttpRequestHandler.match('/chart-(.*).json')
  def _chart(self, name):
    data = self.server.metrics.get(name)
    return self.send_data(200, 'text/svg', data.draw())

  @HttpRequestHandler.match('/data-(.*).json')
  def _json(self, name):
    data = self.server.metrics.get(name)
    return self.send_data(200, 'text/json', data.json())

  @HttpRequestHandler.match('/(.*)')
  def page(self, name):
    path = self.find_path(name)
    if path is None:
      self.handle_not_found()
    else:
      self.send_file(path)

class MonitorWebSocketHandler(WebSocketHandler):
  READ_TIMEOUT = 2.0

  def on_message(self, message):
    print 'GOT MESSAGE', message

  def on_read_timeout(self):
    print 'Web Socket Read Timeout', self.server.metrics
    self.send_message("Hey There")

class R5lMonitor(AbstractService):
  def _setup(self):
    self.metrics = R5lMetrics()

  def _worker_loops(self):
    yield self._websocket_loop
    yield self._webpage_loop

  def update(self):
    self.log_message('Update Monitor Cache', time.time())
    self.metrics.update()

  def _webpage_loop(self):
    self.log_message(' - Start WebPage Loop 8080')
    server = HttpTcpServer(('', 8080), MonitorHttpRequestHandler)
    server.metrics = self.metrics
    self._server_loop(server)

  def _websocket_loop(self):
    self.log_message(' - Start WebSocket Loop 8081')
    server = WebSocketTcpServer(('', 8081), MonitorWebSocketHandler)
    server.metrics = self.metrics
    self._server_loop(server)

if __name__ == '__main__':
  MONITOR_UPDATE_INTERVAL = 2.0
  monitor = R5lMonitor()
  try:
    monitor.start()
    while monitor.is_running():
      monitor.update()
      time.sleep(MONITOR_UPDATE_INTERVAL)
  except KeyboardInterrupt:
    pass
  finally:
    monitor.stop()
