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

from contextlib import contextmanager

import threading
import select

class IOPollEntity(object):
  WATCHED  = (1 << 0)
  READABLE = (1 << 1)
  WRITABLE = (1 << 2)
  HANGUP   = (1 << 3)

  def __init__(self):
    self.flags = self.READABLE
    self._iopoll = None

  def fileno(self):
    raise NotImplementedError

  def close(self):
    raise NotImplementedError

  def read(self):
    raise NotImplementedError

  def write(self):
    raise NotImplementedError

  def is_readable(self):
    return self.flags & self.READABLE

  def is_writable(self):
    return self.flags & self.WRITABLE

  def is_watched(self):
    return self.flags & self.WATCHED

  def set_readable(self, value=True):
    self._set_flag(self.READABLE, value)
    self._iopoll._engine.add(self._iopoll, self)

  def set_writable(self, value=True):
    self._set_flag(self.WRITABLE, value)
    self._iopoll._engine.add(self._iopoll, self)

  def set_watched(self, iopoll, value=True):
    self._set_flag(self.WATCHED, value)
    self._iopoll = iopoll

  def _set_flag(self, flag, value):
    if value:
      self.flags |= flag
    else:
      self.flags &= ~flag

class _EPollEngine(object):
  def open(self, iopoll):
    self._epoll = select.epoll()
    self._lock = threading.Lock()
    self._fdmap = {}

  def close(self, iopoll):
    for entity in self._fdmap.values():
      self.remove(iopoll, entity)
      entity.close()
    self._epoll.close()

  def add(self, iopoll, entity):
    events = select.EPOLLHUP | select.EPOLLERR
    if entity.is_readable():
      events |= select.EPOLLIN
    if entity.is_writable(): events |= select.EPOLLOUT
    fd = entity.fileno()
    if entity.is_watched():
      self._epoll.modify(fd, events)
    else:
      self._set_watched(iopoll, entity, True)
      self._epoll.register(fd, events)

  def remove(self, iopoll, entity):
    if entity.is_watched():
      self._set_watched(iopoll, entity, False)
      self._epoll.unregister(entity.fileno())

  def poll(self, iopoll):
    while iopoll.is_looping:
      events = self._epoll.poll(timeout=1)
      self._lock.acquire()
      fdmap = dict(self._fdmap)
      self._lock.release()
      for fd, eflags in events:
        entity = fdmap.get(fd)
        if entity:
          iopoll.process(entity, self._epoll_event_to_iopoll(eflags))

  def _set_watched(self, iopoll, entity, value):
    fd = entity.fileno()
    self._lock.acquire()
    entity.set_watched(iopoll, value)
    if value:
      self._fdmap[fd] = entity
    else:
      del self._fdmap[fd]
    self._lock.release()

  @staticmethod
  def _epoll_event_to_iopoll(events):
    eflags = 0
    if events & select.EPOLLIN:  eflags |= IOPollEntity.READABLE
    if events & select.EPOLLOUT: eflags |= IOPollEntity.WRITABLE
    if events & select.EPOLLHUP: eflags |= IOPollEntity.HANGUP
    if events & select.EPOLLERR: eflags |= IOPollEntity.HANGUP
    return eflags

class IOPoll(object):
  def __init__(self):
    self._engine = _EPollEngine()
    self.is_looping = False
    self._thread = None

  def open(self):
    self.is_looping = True
    self._engine.open(self)
    self._thread = threading.Thread(target=self._engine.poll, args=(self,), name='IOPoll')
    self._thread.start()

  def close(self):
    self.is_looping = False
    self._engine.close(self)

  def add(self, entity):
    self._engine.add(self, entity)

  def remove(self, entity):
    self._engine.remove(self, entity)

  def process(self, entity, events):
    if events & IOPollEntity.HANGUP:
      print 'Entity HANGUP'
      self._engine.remove(self, entity)
      entity.close()
      return

    if events & IOPollEntity.READABLE:
      try:
        entity.read()
      except Exception as e:
        print e
        self._engine.remove(self, entity)
        entity.close()

    if events & IOPollEntity.WRITABLE:
      try:
        entity.write()
      except Exception as e:
        print e
        self._engine.remove(self, entity)
        entity.close()

  @contextmanager
  def loop(self):
    self.open()
    try:
      yield
    finally:
      self.close()
