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

from threading import Thread

import signal
import grp
import pwd
import sys
import os

class AbstractService(object):
  CLS_REQUEST_HANDLER = None
  CLS_UNIX_SERVER = None
  CLS_TCP_SERVER = None

  def __init__(self):
    self._running = False
    self._threads = []
    self._setup_signals()
    self._setup()

  def set_user(self, user):
    if isinstance(user, basestring):
      _, _, user, gid, _, _, _ = pwd.getpwnam(user)

    if user is not None:
      self._drop_group_privileges()
      os.setuid(user)

  def set_group(self, group):
    if isinstance(group, basestring):
      _, _, group, _ = grp.getgrnam(group)

    if group is not None:
      self._drop_group_privileges()
      os.setgid(group)

  def set_umask(self, umask):
    os.umask(umask)

  def _setup(self):
    pass

  def _worker_loops(self):
    raise NotImplementedError

  def _server_loop(self, server):
    server.is_running = self.is_running
    while self._running:
      server.handle_request()

  def start(self):
    if self._running:
      return

    self.log_message('Start Server')
    for func in self._worker_loops():
      self._threads.append(Thread(target=func))

    self._running = True
    for thread in self._threads:
      thread.daemon = True
      thread.start()

  def stop(self):
    if not self._running:
      return
    self.log_message('Stop Server')
    self._running = False
    for thread in self._threads:
      thread.join(0.5)

  def reload(self):
    pass

  def is_running(self):
    return self._running

  def _sighup(self, signo, frame):
    self.reload()

  def _sigint(self, signo, frame):
    self.stop()

  def _sigterm(self, signo, frame):
    self.stop()

  def _setup_signals(self):
    signal.signal(signal.SIGHUP, self._sighup)
    signal.signal(signal.SIGINT, self._sigint)
    signal.signal(signal.SIGTERM, self._sigterm)

  def _drop_group_privileges(self):
    try:
      os.setgroups([])
    except OSError, exc:
      print 'Failed to remove group privileges: %s' % exc

  def log_message(self, *args):
    msg = ' '.join([str(x) for x in args]) if len(args) > 0 else ''
    sys.stdout.write('%s\n' % msg)
    sys.stderr.flush()
