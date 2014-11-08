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

from sevugi import SvgWriter, SvgPath, ChartData, ChartStyle
from sevugi import PieChart, LineChart, AreaChart, VBarChart

from cStringIO import StringIO
from collections import deque
from datetime import datetime
from time import time

import random

CANVAS_WIDTH = 400
CANVAS_HEIGHT = 200
from random import randint, uniform
from collections import Counter

import sys
sys.path.append("../r5l-client/py-r5l/")

def humanSize(size):
  hs = ('K', 'M', 'G', 'T', 'P', 'E', 'Z')
  count = len(hs)
  while count > 0:
    ref = float(1 << (count * 10))
    count -= 1
    if size >= ref:
      if (size % 1024) == 0:
        return '%d%s' % (size / ref, hs[count])
      return '%.1f%s' % (size / ref, hs[count])
  return '%d' % size

def humanTime(usec):
  if usec >= 60000000:
    return "%.2fM" % (usec / 60000000.0)
  if usec >= 1000000:
    return "%.1fs" % (usec / 1000000.0)
  if usec >= 1000:
    return "%.1fm" % (usec / 1000.0)
  return "%.1fu" % usec;

class _MetricsBase(object):
  def draw(self):
    raise NotImplementedError

  def json(self):
    raise NotImplementedError

class DummyMetrics(object):
  pass

class PieTest(object):
  def draw(self):
    wfile = StringIO()
    svg = SvgWriter(wfile)
    with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
      style = ChartStyle()
      style.setWidth(CANVAS_WIDTH).setHeight(CANVAS_HEIGHT)
      style.setPad(2)

      # Random Data
      data = [uniform(10, 200) for i in xrange(8)]
      labels = ['Data %d = %d' % x for x in enumerate(data)]

      # Generate Graph
      chart = PieChart(svg, style)
      c = CANVAS_HEIGHT / 2
      chart.draw(c, c, data)
      chart.drawLabels(CANVAS_HEIGHT + 5, 5, labels)
    return wfile.getvalue()

class AreaTest(object):
  def __init__(self):
    self._data_seqid = {}
    self._data_chart4 = deque(maxlen=50)

  def draw(self):
    wfile = StringIO()
    svg = SvgWriter(wfile)
    with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
      style = ChartStyle()
      style.setWidth(CANVAS_WIDTH).setHeight(CANVAS_HEIGHT)
      style.setXPad(40).setPad(20)

      # Random Data
      random.seed(time() * 1000)
      seqid = self._data_seqid['chart4'] = self._data_seqid.get('chart4', 0) + 1
      self._data_chart4.append((seqid, int(uniform(0, 400))))

      data = ChartData()
      data.xaxis_label = 'X-axis'
      data.yaxis_label = 'Y-axis'
      #data.add([randint(0, 100) for i in xrange(100)], [0, 20, 10, 30, 20, 50])
      data.addRows(None, self._data_chart4)

      # Generate Graph
      chart = AreaChart(svg, style)
      chart.draw(35, 20, data)
    return wfile.getvalue()

class _DataChart(object):
  CANVAS_SIZE = (400, 200)
  XAXIS_FORMAT = None
  YAXIS_FORMAT = lambda self, x: humanSize(x)
  XAXIS_LABEL = None
  YAXIS_LABEL = None
  CANVAS_XPAD = 40
  CANVAS_PAD = 20

  def update(self, data):
    raise NotImplementedError

  def draw(self):
    raise NotImplementedError

  def drawData(self):
    raise NotImplementedError

class _DataMultilineTimeSeries(_DataChart):
  XAXIS_FORMAT = lambda self, x: humanTime(x)
  XAXIS_LABEL = 'Time'
  CANVAS_XPAD = 40
  NITEMS = 256

  def __init__(self):
    self.timeline = deque(maxlen=self.NITEMS)
    self.seqid = 0
    self.series = {}

  def update(self, data):
    self.timeline.append(self.seqid)
    self.seqid += 1

    kseries = set(self.series.keys()) | set(data.keys())
    for k in kseries:
      kdata = self.series.get(k, None)
      if kdata is None:
        kdata = deque([data.get(k, 0)] * (len(self.timeline)), maxlen=self.NITEMS)
        self.series[k] = kdata
      else:
        kdata.append(data.get(k, 0))

  def draw(self):
    wfile = StringIO()
    svg = SvgWriter(wfile)
    width, heigh = self.CANVAS_SIZE
    with svg.canvas(width, heigh):
      style = ChartStyle()
      style.setWidth(width).setHeight(heigh)
      style.setXPad(self.CANVAS_XPAD).setPad(self.CANVAS_PAD)
      style.setXAxisFormat(self.XAXIS_FORMAT)
      style.setYAxisFormat(self.YAXIS_FORMAT)

      data = self.drawData()
      data.xaxis_label = self.XAXIS_LABEL
      data.yaxis_label = self.YAXIS_LABEL

      # Generate Graph
      chart = LineChart(svg, style)
      chart.drawLabels(style.dataWidth() + 10, 1, data.labels)
      chart.draw(35, 20, data)
    return wfile.getvalue()

  def drawData(self):
    data = ChartData()
    for k, kserie in self.series.iteritems():
      data.add(k, self.timeline, kserie)
    return data

class _DeltaDataMultilineTimeSeries(_DataMultilineTimeSeries):
  def drawData(self):
    data = ChartData()
    last_value = None
    for k, kserie in self.series.iteritems():
      delta_kserie = []
      for i in xrange(1, len(kserie)):
        delta_kserie.append(kserie[i] - kserie[i-1])
      data.add(k, self.timeline, delta_kserie)
    return data

class _DataBarChart(_DataChart):
  X_STEP = 25
  Y_STEP = 20

  def __init__(self):
    self.histogram = {}

  def update(self, data):
    self.histogram.update(data)

  def draw(self, tail=False):
    wfile = StringIO()
    svg = SvgWriter(wfile)
    width, heigh = self.CANVAS_SIZE
    with svg.canvas(width, heigh):
      style = ChartStyle()
      style.setWidth(width).setHeight(heigh)
      style.setXPad(self.CANVAS_XPAD).setPad(self.CANVAS_PAD)
      style.setXAxisFormat(self.XAXIS_FORMAT)
      style.setYAxisFormat(self.YAXIS_FORMAT)

      if tail:
        data = self.drawTailData()
      else:
        data = self.drawData()
      data.xaxis_label = self.XAXIS_LABEL
      data.yaxis_label = self.YAXIS_LABEL

      # Generate Graph
      chart = VBarChart(svg, style)
      chart.draw(self.X_STEP, self.Y_STEP, data)
    return wfile.getvalue()

class DataCtxSwitch(_DataMultilineTimeSeries):
  YAXIS_LABEL = 'ctx-switch'

class DataIo(_DataMultilineTimeSeries):
  YAXIS_LABEL = 'io'

class DataMemUsage(_DataMultilineTimeSeries):
  YAXIS_LABEL = 'Mem'

class DataMemReqs(_DataMultilineTimeSeries):
  YAXIS_LABEL = 'Mem-Reqs'

class DataLoad(_DataMultilineTimeSeries):
  CANVAS_SIZE = (780, 200)
  YAXIS_FORMAT = lambda self, x: humanTime(x)
  YAXIS_LABEL = 'Time'

class DataLatency(_DataBarChart):
  XAXIS_FORMAT = lambda self, x: humanTime(x)
  YAXIS_FORMAT = lambda self, x: humanSize(x)
  CANVAS_SIZE = (780, 200)
  XAXIS_LABEL = 'Latency'
  YAXIS_LABEL = 'Reqs'
  X_STEP = 32

  def drawData(self):
    data = ChartData()
    xdata = []
    ydata = []
    for k in sorted(self.histogram.iterkeys()):
      xdata.append(self.XAXIS_FORMAT(k))
      ydata.append(self.histogram[k])
    data.add(None, xdata, ydata)
    return data

  def drawTailData(self):
    total = max(self.histogram.itervalues())
    data = ChartData()
    xdata = []
    ydata = []
    for k in sorted(self.histogram.iterkeys()):
      yvalue = self.histogram[k]
      if (float(yvalue) / total) < 0.5:
        xdata.append(self.XAXIS_FORMAT(k))
        ydata.append(yvalue)
    data.add(None, xdata, ydata)
    return data

class DataMemPool(_DataBarChart):
  XAXIS_FORMAT = lambda self, x: humanSize(x)
  YAXIS_FORMAT = lambda self, x: humanSize(x)
  CANVAS_SIZE = (780, 200)
  XAXIS_LABEL = 'Blk-Size'
  YAXIS_LABEL = 'Reqs'

  def drawData(self):
    data = ChartData()
    xdata = []
    ydata = []
    for k in sorted(self.histogram.iterkeys()):
      xdata.append(self.XAXIS_FORMAT(k))
      ydata.append(self.histogram[k])
    data.add(None, xdata, ydata)
    return data

from contextlib import contextmanager
class StatsClient(object):
  @contextmanager
  def connection(self, *args, **kwargs):
    yield

  def rusage(self):
    return {
      'utime': randint(0, 7000),
      'stime': randint(0, 7000),
      'maxrss': randint(0, 7000),
      'minflt': randint(0, 7000),
      'majflt': randint(0, 7000),
      'inblock': randint(0, 7000),
      'oublock': randint(0, 7000),
      'nvcsw': randint(0, 7000),
      'nivcsw': randint(0, 7000),
    }

  def memory(self):
    return {
      'pool_used': randint(0, 7000),
      'pool_alloc': randint(0, 7000),
      'extern_used': randint(0, 7000),
      'extern_alloc': randint(0, 7000),
      'histogram': self._mem_histo(),
    }

  def iopoll(self):
    return {
      'io_wait': self._latency_histo(),
      'io_read': self._latency_histo(),
      'io_write': self._latency_histo(),
      'event': self._latency_histo(),
      'timer': self._latency_histo(),
    }

  def _latency_histo(self):
    return {i * 10: randint(0, 1000) for i in xrange(20)}

  def _mem_histo(self):
    return {1 << i: randint(0, 1000) for i in xrange(28)}

def filter_dict(kvdata, filter_keys):
  return {k: v for k, v in kvdata.iteritems() if k in filter_keys}

def histogram_from_json(json):
  return dict(zip(json['bounds'], json['events']))

class R5lMetrics(object):
  def __init__(self):
    self._dummy_metrics = DummyMetrics()
    self._version = 0
    self._cache = {
      # rusage
      'ctxswitch': DataCtxSwitch(),
      'io': DataIo(),

      # iopoll
      'ioload': DataLoad(),
      'iowait': DataLatency(),
      'ioread': DataLatency(),
      'iowrite': DataLatency(),
      'event': DataLatency(),
      'timer': DataLatency(),

      # memory
      'memusage': DataMemUsage(),
      'memreqs': DataMemReqs(),
      'mempool': DataMemPool(),

      'pietest': PieTest(),
      'areatest': AreaTest(),
    }

  def __repr__(self):
    return 'R5lMetrics(version %d)' % self._version

  def get(self, name):
    return self._cache.get(name, self._dummy_metrics)

  def update(self):
    self._version += 1

    CTX_SWITCH_KEYS = ('nvcsw', 'nivcsw')
    IO_LOAD_KEYS = ('idle', 'active')
    IO_KEYS = ('inblock', 'outblock')
    try:
      from metrics import MetricsClient

      #client = StatsClient()
      client = MetricsClient()
      with client.connection(None, '127.0.0.1', 11213):
        # RUsage
        try:
          rusage_data = client.rusage()
          self._cache['ctxswitch'].update(filter_dict(rusage_data, CTX_SWITCH_KEYS))
          self._cache['io'].update(filter_dict(rusage_data, IO_KEYS))
        except Exception as e:
          print 'rusage exception', e

        # Client
        try:
          iopoll = client.iopoll(0)
          for idle, active in zip(iopoll['io_load']['idle'], iopoll['io_load']['active']):
            self._cache['ioload'].update({'idle': idle, 'active': active})
          self._cache['iowait'].update(histogram_from_json(iopoll['io_wait']))
          self._cache['iowrite'].update(histogram_from_json(iopoll['io_read']))
          self._cache['ioread'].update(histogram_from_json(iopoll['io_write']))
          self._cache['event'].update(histogram_from_json(iopoll['event']))
          self._cache['timer'].update(histogram_from_json(iopoll['timer']))
        except Exception as e:
          print 'iopoll exception', e

        # Memory
        try:
          memory = client.memory(0)
          self._cache['memusage'].update({'pool': memory['pool_used'], 'extern': memory['extern_used']})
          self._cache['memreqs'].update({'pool': memory['pool_alloc'], 'extern': memory['extern_alloc']})
          self._cache['mempool'].update(histogram_from_json(memory['histogram']))
        except Exception as e:
          print 'memory exception', e
    except Exception as e:
      print 'FUNK', e
