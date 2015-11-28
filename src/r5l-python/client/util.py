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

def humanSize(size):
  hs = ('KiB', 'MiB', 'GiB', 'TiB', 'PiB')
  count = len(hs)
  while count > 0:
    ref = float(1 << (count * 10))
    count -= 1
    if size >= ref:
      return '%.2f%s' % (size / ref, hs[count])
  return '%dbytes' % size

def humanTime(nsec):
  if nsec >= 60000000000:
    return "%.2fmin" % (nsec / 60000000000.0)
  if nsec >= 1000000000:
    return "%.2fsec" % (nsec / 1000000000.0)
  if nsec >= 1000000:
    return "%.2fmsec" % (nsec / 1000000.0)
  if nsec >= 1000:
    return "%.2fusec" % (nsec / 1000.0)
  return '%.2fnsec' % nsec

class Histogram(object):
  def __init__(self, bounds, events=None):
    self.bounds = bounds
    if events:
      self.events = events
      self.max = self.bounds[-1]
    else:
      self.events = [0] * len(bounds)
      self.max = 0

  @staticmethod
  def from_flat(data):
    n = len(data) / 2
    bounds = [0] * n
    events = [0] * n
    for i in xrange(n):
      index = i * 2
      bounds[i] = data[index+0]
      events[i] = data[index+1]
    return Histogram(bounds, events)

  @staticmethod
  def from_diff(current, previous):
    p_map = dict(x for x in zip(previous.bounds, previous.events))
    c_map = dict(x for x in zip(current.bounds[:-1], current.events))
    for k in c_map:
      c_map[k] = max(0, c_map.get(k, 0) - p_map.get(k, 0))
    if not previous.bounds[-1] in c_map:
      c_map[current.bounds[-1]] = current.events[-1] - previous.events[-1]
    bounds = sorted(c_map.keys())
    events = [c_map[k] for k in bounds]
    return Histogram(bounds, events)

  def add(self, value):
    bounds = self.bounds
    for i in xrange(len(bounds)):
      if value <= bounds[i]:
        self.events[i] += 1
        break
    self.max = max(self.max, value)

  def nevents(self):
    return sum(self.events)

  def percentile(self, p):
    nevents = self.nevents()
    threshold = nevents * (p * 0.01)
    xsum = 0
    for i in xrange(len(self.bounds)):
      xsum += self.events[i]
      if xsum >= threshold:
        # Scale linearly within this bucket
        left_point = self.bounds[0 if (i == 0) else (i - 1)]
        right_point = self.bounds[i]
        left_sum = xsum - self.events[i]
        right_sum = xsum
        pos = 0
        right_left_diff = right_sum - left_sum
        if right_left_diff != 0:
          pos = (threshold - left_sum) / right_left_diff
        r = left_point + (right_point - left_point) * pos
        return min(r, self.max)
    return self.max
