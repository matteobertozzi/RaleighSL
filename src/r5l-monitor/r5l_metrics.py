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

def humanTime(time):
  return '%d' % time

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

  def __init__(self):
    self.timeline = deque(maxlen=60)
    self.seqid = 0
    self.series = {}

  def update(self, data):
    self.timeline.append(self.seqid)
    self.seqid += 1

    kseries = set(self.series.keys()) | set(data.keys())
    for k in kseries:
      kdata = self.series.get(k, None)
      if kdata is None:
        kdata = deque([data.get(k, 0)] * (len(self.timeline)), maxlen=60)
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

class _DataBarChart(_DataChart):
  def __init__(self):
    self.histogram = {}

  def update(self, data):
    self.histogram.update(data)

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
      chart = VBarChart(svg, style)
      chart.draw(25, 20, data)
    return wfile.getvalue()

class DataCtxSwitch(_DataMultilineTimeSeries):
  YAXIS_LABEL = 'ctx-switch'

class DataIoPoll(_DataMultilineTimeSeries):
  YAXIS_LABEL = 'events'

class DataIo(_DataMultilineTimeSeries):
  YAXIS_LABEL = 'io'

class DataMemUsage(_DataMultilineTimeSeries):
  YAXIS_LABEL = 'Mem'

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
    return {'nvcsw': randint(0, 7000),
            'nivcsw': randint(0, 7000),
            'inblock': randint(0, 7000),
            'outblock': randint(0, 7000),
            'iowait': randint(0, 7000),
            'ioread': randint(0, 7000),
            'iowrite': randint(0, 7000)}

  def memusage(self):
    return {'sys-used': randint(0, 7000),
            'sys-free': randint(0, 8000)}

  def mempool(self):
    histogram = {}
    for i in xrange(28):
      histogram[1 << i] = randint(0, 1000)
    return histogram

def filter_dict(kvdata, filter_keys):
  return {k: v for k, v in kvdata.iteritems() if k in filter_keys}

class R5lMetrics(object):
  def __init__(self):
    self._dummy_metrics = DummyMetrics()
    self._version = 0
    self._cache = {
      'ctxswitch': DataCtxSwitch(),
      'io': DataIo(),
      'iopoll': DataIoPoll(),
      'memusage': DataMemUsage(),
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
    IOPOLL_KEYS = ('iowrite', 'ioread', 'iowrite')
    IO_KEYS = ('inblock', 'outblock')
    try:
      client = StatsClient()
      with client.connection(None, '127.0.0.1', 11217):
        # RUsage
        rusage_data = client.rusage()
        self._cache['ctxswitch'].update(filter_dict(rusage_data, CTX_SWITCH_KEYS))
        self._cache['iopoll'].update(filter_dict(rusage_data, IOPOLL_KEYS))
        self._cache['io'].update(filter_dict(rusage_data, IO_KEYS))

        # Mem-Usage
        self._cache['memusage'].update(client.memusage())

        # Mem-Pool
        self._cache['mempool'].update(client.mempool())
    except Exception as e:
      print 'FUNK', e
