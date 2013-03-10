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

import threading
import select
import socket

class IOPoll(object):
  MAX_DELAY = 0.050
  BACKOFF = 0.00001
  TIMEOUT = 0.00001

  def __init__(self):
    self._lock = threading.Lock()
    self._running = False
    self._thread = None
    self._fds = set()

  def start(self):
    if self._running == True:
      raise Exception('IOPoll is already running')
    self._running = True
    self._thread = threading.Thread(target=self._poll, name='IOPoll')
    self._thread.start()

  def stop(self, wait=True, timeout=None):
    if self._running == False:
      raise Exception('IOPoll is not running')
    self._running = False
    if wait: self._thread.join(timeout)
    self._thread = None

  def _poll(self):
    timeout = self.TIMEOUT
    while self._running == True:
      fds_map = {obj.fileno(): obj for obj in self._fds if obj.fileno() >= 0}
      fds = fds_map.keys()
      wfds = [fd for fd, obj in fds_map.iteritems() if obj.is_writable()]
      rfds, wfds, efds = select.select(fds, wfds, fds, timeout)
      if len(rfds) == 0 and len(wfds) == 0 and len(efds) == 0:
        timeout = min(timeout + self.BACKOFF, self.MAX_DELAY)
        continue
      timeout = self.TIMEOUT
      fds = set()
      # Exceptions
      for fd in efds:
        fds_map[fd].close()
        print 'got exception', fd
        fds.add(fd)
      # Reads
      for fd in rfds:
        if not fd in fds:
          try:
            fds_map[fd].read()
          except socket.error as e:
            if e.errno == 11:
              print 'got exception while reading, retry', fd, e
            else:
              fds.add(fd)
          except Exception as e:
            print 'got exception while reading', fd, type(e), e
            fds.add(fd)
      # Writes
      for fd in wfds:
        if not fd in fds:
          try:
            fds_map[fd].write()
          except Exception as e:
            print 'got exception while writing', fd, e
            fds.add(fd)
      # Removes closed fds
      if len(fds) > 0:
        self._lock.acquire()
        try:
          for fd in fds:
            self._fds.discard(fds_map[fd])
        finally:
          self._lock.release()

  def __contains__(self, o):
    return o in self._fds

  def add(self, fd):
    self._lock.acquire()
    try:
      self._fds.add(fd)
    finally:
      self._lock.release()

  def remove(self, fd):
    self._lock.acquire()
    try:
      self._fds.discard(fd)
    finally:
      self._lock.release()
