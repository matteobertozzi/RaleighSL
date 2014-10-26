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

def humanTime(usec):
  if usec >= 60000000:
    return "%.2fmin" % (usec / 60000000.0)
  if usec >= 1000000:
    return "%.2fsec" % (usec / 1000000.0)
  if usec >= 1000:
    return "%.2fmsec" % (usec / 1000.0)
  return "%.2fusec" % usec;

class Histogram(object):
  def __init__(self, name, bounds, events):
    self.name = name
    self.bounds = bounds
    self.events = events

  def percentile(self, p):
    nevents = sum(e for e in self.events)
    threshold = nevents * (p * 0.01)
    esum = 0
    for i in xrange(len(self.bounds)):
      esum += self.events[i]
      if esum >= threshold:
        left_point = self.bounds[0] if i == 0 else self.bounds[i - 1]
        right_point = self.bounds[i]
        left_sum = esum - self.events[i]
        right_sum = esum
        pos = 0
        right_left_diff = right_sum - left_sum
        if right_left_diff != 0:
          pos = (threshold - left_sum) / right_left_diff
        r = left_point + (right_point - left_point) * pos
        return self.bounds[-1] if r > self.bounds[-1] else r
    return self.bounds[-1]

  def dump(self, human_key):
    if not self.events:
      return
    emin = min(e for e in self.events)
    emax = max(e for e in self.events)

    mult = 100.0 / sum(e for e in self.events)
    print '%s - MAX %s Percentiles: %s' % (self.name, human_key(self.bounds[-1]), ' '.join(['P%s %s' % (p, human_key(self.percentile(p))) for p in (50, 75, 99, 99.9, 99.99)]))
    for bound, events in zip(self.bounds, self.events):
      barlen = int((float(events) / max(1, emax)) * 60)
      print '%10s (%5.2f%%) | %s (%d events)' % (human_key(bound), mult * events, '=' * barlen, events)

  @staticmethod
  def from_json(name, histo_json):
    return Histogram(name, histo_json['bounds'], histo_json['events'])
