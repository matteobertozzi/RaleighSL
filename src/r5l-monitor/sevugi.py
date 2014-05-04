#!/usr/bin/env python
#
#   Copyright 2014 Matteo Bertozzi
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
from math import ceil

import itertools
import random
import math

def minmax(data):
  # http://code.activestate.com/recipes/577916-fast-minmax-function/
  it = iter(data)
  try:
    lo = hi = next(it)
  except StopIteration:
    raise ValueError('minmax() arg is an empty sequence')
  for x, y in itertools.izip_longest(it, it, fillvalue=lo):
    if x > y:
      x, y = y, x
    if x < lo:
      lo = x
    if y > hi:
      hi = y
  return lo, hi

def multi_minmax(data):
  return minmax(itertools.chain(*data))

class SvgStyle(object):
  def __init__(self, other=None):
    self.attr = {}
    if other is not None:
      self.merge(other)

  def merge(self, style):
    assert isinstance(style, SvgStyle)
    self.attr.update(style.attr)
    return self

  def __str__(self):
    return '; '.join(['%s: %s' % x for x in self.attr.iteritems()])

  def addAttr(self, key, value):
    self.attr[key] = value
    return self

  def fill(self, color):
    return self.addAttr('fill', color)

  def fillOpacity(self, opacity):
    return self.addAttr('fill-opacity', opacity)

  def stroke(self, color):
    return self.addAttr('stroke', color)

  def strokeWidth(self, width):
    return self.addAttr('stroke-width', width)

  def strokeDashArray(self, dashArray):
    return self.addAttr('stroke-dasharray', ' '.join(['%d' % x for x in dashArray]))

  def fontSize(self, size):
    return self.addAttr('font-size', size)

  def fontStyle(self, style):
    return self.addAttr('font-style', style)

  def textAnchor(self, anchor):
    return self.addAttr('text-anchor', anchor)

  def writingMode(self, mode):
    return self.addAttr('writing-mode', mode)

  def glyphOrientationVertical(self, rot):
    return self.addAttr('glyph-orientation-vertical', rot)

  def glyphOrientationHorizontal(self, rot):
    return self.addAttr('glyph-orientation-horizontal', rot)

class SvgPath(object):
  # http://tutorials.jenkov.com/svg/path-element.html

  def __init__(self):
    self.path = []

  def __str__(self):
    return ' '.join(self.path)

  def _add(self, x):
    self.path.append(x)
    return self

  def moveTo(self, x, y):
    return self._add('M%d,%d' % (x, y))

  def lineTo(self, x, y):
    return self._add('L%d,%d' % (x, y))

  def horizontalLineTo(self, x):
    return self._add('H%d' % x)

  def verticalLineTo(self, y):
    return self._add('V%d' % y)

  def curveTo(self, x, y):
    return self._add('T%d,%d' % (x, y))

  def arcTo(self, rx, ry, x, y):
    return self._add('A%d,%d 0 0,1 %d,%d' % (rx, ry, x, y))

  def close(self):
    return self._add('z')

class SvgWriter(object):
  # http://commons.oreilly.com/wiki/index.php/SVG_Essentials/Getting_Started
  # http://commons.oreilly.com/wiki/index.php/SVG_Essentials/Basic_Shapes

  def __init__(self, writer):
    self.write = writer.write

  @contextmanager
  def canvas(self, width, height):
    self.write('<svg width="%d" height="%d">' % (width, height))
    yield
    self.write('</svg>')

  @contextmanager
  def group(self, x, y):
    self.write('<g transform="translate(%d,%d)">' % (x, y))
    yield
    self.write('</g>')

  def _add(self, x):
    self.write(x)
    return self

  def line(self, x1, y1, x2, y2, style):
    return self._add('<line x1="%d" y1="%d" x2="%d" y2="%d" style="%s"/>' % (x1, y1, x2, y2, style))

  def rectangle(self, x, y, width, height, style):
    return self._add('<rect x="%d" y="%d" width="%d" height="%d" style="%s" />' % (x, y, width, height, style))

  def roundedRect(self, x, y, width, height, rx, style):
    return self._add('<rect x="%d" y="%d" width="%d" height="%d" rx="%d" style="%s" />' % (x, y, width, height, rx, style))

  def circle(self, cx, cy, r, style):
    return self._add('<circle cx="%d" cy="%d" r="%d" style="%s"/>' % (cx, cy, r, style))

  def path(self, path, style):
    return self._add('<path d="%s" style="%s"/>' % (path, style))

  def text(self, x, y, text, style):
    return self._add('<text x="%d" y="%d" style="%s">%s</text>' % (x, y, style, text))

  def verticalText(self, x, y, text, style):
    vstyle = SvgStyle().writingMode('tb').glyphOrientationVertical(90)
    return self.text(x, y, text, vstyle.merge(style))

class ChartData(object):
  def __init__(self):
    self.xaxis_label = None
    self.yaxis_label = None
    self.labels = []
    self.xdata = []
    self.ydata = []

  def add(self, label, xdata, ydata):
    self.labels.append(label)
    self.xdata.append(xdata)
    self.ydata.append(ydata)

  def addRows(self, label, rows):
    xdata = []
    ydata = []
    for x, y in rows:
      xdata.append(x)
      ydata.append(y)
    return self.add(label, xdata, ydata)

  def xminmax(self):
    return multi_minmax(self.xdata)

  def yminmax(self):
    return multi_minmax(self.ydata)

class ChartStyle(object):
  def __init__(self):
    self.xaxis_format = None
    self.yaxis_format = None

  def setWidth(self, width):
    self.width = width
    return self

  def setHeight(self, height):
    self.height = height
    return self

  def setXPad(self, xpad):
    self.xpad = xpad
    return self

  def setPad(self, pad):
    self.pad = pad
    return self

  def setXAxisFormat(self, format):
    self.xaxis_format = format
    return self

  def xAxisFormatConvert(self, labels):
    if self.xaxis_format:
      return [self.xaxis_format(x) for x in labels]
    return labels

  def setYAxisFormat(self, format):
    self.yaxis_format = format
    return self

  def yAxisFormatConvert(self, labels):
    if self.yaxis_format:
      return [self.yaxis_format(x) for x in labels]
    return labels

  def dataHeigh(self):
    return float(self.height - (self.pad * 2))

  def dataWidth(self):
    return float(self.width - (self.xpad + self.pad))

class Chart(object):
  COLORS1 = ['#3366cc', '#dc3912', '#ff9900', '#109618', '#990099',
             '#0099c6', '#dd4477', '#66aa00', '#b82e2e', '#316395',
             '#994499', '#22aa99', '#aaaa11', '#6633cc', '#16d620']

  COLORS2 = ['#e6e6e6', '#cdcdcd', '#b4b4b4', '#9b9b9b', '#828282',
             '#696969', '#505050', '#373737', '#1f1f1f', '#050505']

  COLORS3 = ['#7f2704', '#a63603', '#cc4c02', '#d94801', '#f16913',
             '#fd8d3c', '#fdae6b', '#fec44f', '#fdd0a2', '#fee391']

  COLORS3 = ['#7f2704', '#fee391', '#a63603', '#fdd0a2', '#cc4c02',
             '#fec44f', '#d94801', '#fdae6b', '#f16913', '#fd8d3c']

  def __init__(self, svg, style):
    self.svg = svg
    self.style = style

  def _colors(self):
    return itertools.cycle(self.COLORS3)

  def draw(self, xstep, ystep, data):
    raise NotImplementedError

  def drawBase(self, xaxis_label, xstep, xlabels, yaxis_label, ystep, ylabels):
    style = self.style
    Chart._drawAxis(self.svg, style.width, style.height, style.xpad, style.pad, xaxis_label, yaxis_label)
    Chart._drawYAxis(self.svg, style.width, style.height, style.xpad, style.pad, ystep, ylabels)
    Chart._drawXAxis(self.svg, style.width, style.height, style.xpad, style.pad, xstep, xlabels)

  def drawLabels(self, x, y, labels):
    with self.svg.group(x, y):
      y = 0
      for label, color in zip(labels, self._colors()):
        self.svg.rectangle(0, y + 1, 7, 7, SvgStyle().stroke('#555').fill(color))
        self.svg.text(11, y + 7, label, SvgStyle().fontSize(9).fill('999aa0'))
        y += 9

  @staticmethod
  def _drawAxis(svg, width, height, xpad, pad, xlabel, ylabel):
    assert pad >= 10

    styleGrayLine = SvgStyle().stroke('#dcdee0')
    axisText = SvgStyle().fontSize(10).fill('#999aa0').fontStyle('italic')

    yt = pad
    yb = height - pad
    xl = xpad
    xr = width - pad

    # Y-Line
    svg.line(xl, yt, xl - 2,  yt + 5, styleGrayLine)
    svg.line(xl, yt, xl + 2,  yt + 5, styleGrayLine)
    svg.line(xl, yt, xl, yb + 5, styleGrayLine)
    svg.verticalText(xl + 10, yt, ylabel, SvgStyle(axisText).textAnchor('start'))

    # X-Line
    svg.line(xl - 5, yb, xr, yb, styleGrayLine)
    svg.line(xr, yb, xr - 5, yb - 2, styleGrayLine)
    svg.line(xr, yb, xr - 5, yb + 2, styleGrayLine)
    svg.text(xr, height - 2, xlabel, SvgStyle(axisText).textAnchor('end'))

  @staticmethod
  def _drawYAxis(svg, width, height, xpad, pad, step_size, ylabels):
    styleLightLine = SvgStyle().stroke('#efefef').strokeDashArray([3, 3])
    styleGrayLine = SvgStyle().stroke('#dcdee0')
    axisText = SvgStyle().fontSize(9).fill('#999aa0').fontStyle('italic')

    seg_size = 6
    yt = pad
    yb = height - (pad + 5)
    xl = xpad - (seg_size / 2) - 1
    xr = xl + seg_size
    xe = width - pad

    for i, text in enumerate(ylabels):
      ys = yt + (len(ylabels) - i) * step_size
      svg.line(xr, ys, xe, ys, styleLightLine)
      svg.line(xl, ys, xr, ys, styleGrayLine)
      svg.text(xl, ys, text, SvgStyle(axisText).textAnchor('end'))

  @staticmethod
  def _drawXAxis(svg, width, height, xpad, pad, step_size, xlabels):
    styleGrayLine = SvgStyle().stroke('#dcdee0')
    axisText = SvgStyle().fontSize(9).fill('#999aa0').fontStyle('italic')

    seg_size = 6
    yt = (height - pad) - (seg_size / 2) - 1
    yb = yt + seg_size
    xl = xpad - (seg_size / 2) - 1
    xr = xl + seg_size

    for i, text in enumerate(xlabels):
      xs = xl + (i + 1) * step_size
      svg.line(xs, yt, xs, yb, styleGrayLine)
      svg.text(xs, yb + 8, text, SvgStyle(axisText).textAnchor('middle'))

class LineChart(Chart):
  DRAW_AREA = False

  def draw(self, xstep, ystep, data):
    x_min_value, x_max_value = data.xminmax()
    x_space = self.style.dataWidth() / max(1, (x_max_value - x_min_value))
    x_steps = int(ceil(self.style.dataWidth() / xstep))
    x_delta = max(1, (x_max_value - x_min_value) / x_steps)
    xlabels = [x_min_value + ((i + 1) * x_delta) for i in xrange(x_steps - 1)]
    xlabels = self.style.xAxisFormatConvert(xlabels)
    xpos = lambda x: ((x - x_min_value) * x_space)

    y_min_value, y_max_value = data.yminmax()
    y_space = self.style.dataHeigh() / max(1, (y_max_value - y_min_value))
    y_steps = max(1, int(ceil(self.style.dataHeigh() / ystep)))
    y_delta = (y_max_value - y_min_value) / y_steps
    ylabels = [y_min_value + ((i + 1) * y_delta) for i in xrange(y_steps - 1)]
    ylabels = self.style.yAxisFormatConvert(ylabels)
    ypos = lambda y: self.style.dataHeigh() - ((y - y_min_value) * y_space)

    self.drawBase(data.xaxis_label, xstep, xlabels, data.yaxis_label, ystep, ylabels)
    for xvec, yvec, color in zip(data.xdata, data.ydata, self._colors()):
      path = SvgPath()
      if self.DRAW_AREA:
        stroke_color = '#444'
        fill_color = color

        path.moveTo(0, self.style.dataHeigh())
        for x, y in zip(xvec, yvec):
          path.lineTo(xpos(x), ypos(y))
        path.lineTo(xpos(xvec[-1]), self.style.dataHeigh())
        path.close()
      else:
        stroke_color = color
        fill_color = 'none'

        path.moveTo(xpos(xvec[0]), ypos(yvec[0]))
        for x, y in zip(xvec, yvec):
          path.lineTo(xpos(x), ypos(y))

      with self.svg.group(self.style.xpad, self.style.pad):
        self.svg.path(path, SvgStyle().strokeWidth(1.5).stroke(stroke_color).fill(fill_color))

class AreaChart(LineChart):
  DRAW_AREA = True

class VBarChart(Chart):
  def draw(self, xstep, ystep, data):
    axisText = SvgStyle().fontSize(10).fill('#999aa0').fontStyle('italic')

    y_min_value, y_max_value = data.yminmax()
    y_space = self.style.dataHeigh() / max(1, (y_max_value - y_min_value))
    y_steps = int(ceil(self.style.dataHeigh() / ystep))
    y_delta = max(1, (y_max_value - y_min_value) / y_steps)
    ylabels = [y_min_value + ((i + 1) * y_delta) for i in xrange(y_steps - 1)]
    ylabels = self.style.yAxisFormatConvert(ylabels)
    ypos = lambda y: self.style.dataHeigh() - ((y - y_min_value) * y_space)

    xlabels = data.xdata[0]
    ydata = data.ydata[0]
    self.drawBase(data.xaxis_label, xstep, xlabels, data.yaxis_label, ystep, ylabels)
    for i, (xlabel, yvalue, color) in enumerate(zip(xlabels, ydata, self._colors())):
      x = 5 + self.style.xpad + (i * xstep) + xstep / 2
      y = self.style.pad + ypos(yvalue)
      h = self.style.dataHeigh() - ypos(yvalue)
      w = xstep / 2
      self.svg.rectangle(x, y, w, h, SvgStyle().fill(color).stroke('#555'))
      #self.svg.verticalText(x + 8, y + h, xlabel, SvgStyle(axisText).textAnchor('end'))

class PieChart(Chart):
  def draw(self, cx, cy, values):

    vtotal = float(sum(values))
    angles = [ceil(360 * v/vtotal) for v in values]

    r = min(cx, cy) - self.style.pad

    start_angle = 0.0
    end_angle = 0.0
    for v, angle, color in zip(values, angles, self._colors()):
      end_angle = start_angle + angle

      x1 = int(round(cx + r * math.cos(math.pi * start_angle / 180.0)))
      y1 = int(round(cy + r * math.sin(math.pi * start_angle / 180.0)))
      x2 = int(round(cx + r * math.cos(math.pi * end_angle / 180.0)))
      y2 = int(round(cy + r * math.sin(math.pi * end_angle / 180.0)))

      path = SvgPath()
      path.moveTo(cx, cy)
      path.lineTo(x1, y1)
      path.arcTo(r, r, x2, y2)
      path.close()
      self.svg.path(path, SvgStyle().fill(color).stroke('#555'))

      start_angle = end_angle

if __name__ == '__main__':
  from random import randint
  from time import time

  import SimpleHTTPServer
  import SocketServer

  PORT = 8080

  class Proxy(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):
      self.send_response(200)
      self.end_headers()
      self.wfile.write('<html>')
      self.wfile.write('<body>')

      CANVAS_WIDTH = 600
      CANVAS_HEIGHT = 200

      svg = SvgWriter(self.wfile)
      """
      # Svg Multi-Line Chart
      with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
        style = ChartStyle()
        style.setWidth(CANVAS_WIDTH).setHeight(CANVAS_HEIGHT)
        style.setXPad(60).setPad(20)

        # Random Data
        random.seed(time() * 1000)
        data = ChartData()
        data.xaxis_label = 'x-axis'
        data.yaxis_label = 'Y-axis'
        #data.add('Data 0', range(100, 100 + len(ydata[0])), [0, 20, 10, 30, 20, 50])
        #data.add('Data 1', range( 99,  99 + len(ydata[1])), [10, 5, 20, 40, 10, 30])
        #data.add('Data 2', range(100, 100 + len(ydata[2])), [2, 23, 14, 32, 24, 56])
        data.add('Data 0', range(100, 100 + 9), [randint(200, 800) for i in xrange(9)])
        data.add('Data 1', range( 99,  99 + 9), [randint(200, 800) for i in xrange(9)])
        data.add('Data 2', range(100, 100 + 9), [randint(200, 800) for i in xrange(9)])

        # Generate Graph
        chart = LineChart(svg, style)
        chart.drawLabels(1, 20, data.labels)
        chart.draw(35, 20, data)

      # Svg Line-Chart
      with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
        style = ChartStyle()
        style.setWidth(CANVAS_WIDTH).setHeight(CANVAS_HEIGHT)
        style.setXPad(40).setPad(20)

        # Random Data
        random.seed(time() * 1000)
        data = ChartData()
        data.xaxis_label = 'X-axis'
        data.yaxis_label = 'Y-axis'
        #data.add([randint(0, 100) for i in xrange(100)], [0, 20, 10, 30, 20, 50])
        data.add(None, range(100, 200), [randint(0, 100) for i in xrange(100)])

        # Generate Graph
        chart = LineChart(svg, style)
        chart.draw(35, 20, data)

      # Svg VBar Char
      with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
        style = ChartStyle()
        style.setWidth(CANVAS_WIDTH).setHeight(CANVAS_HEIGHT)
        style.setXPad(40).setPad(20)

        # Random Data
        random.seed(time() * 1000)
        data = ChartData()
        data.xaxis_label = 'X-axis'
        data.yaxis_label = 'Y-axis'
        data.add(None, ['Data-%d' % i for i in xrange(9)], [randint(200, 800) for i in xrange(9)])

        # Generate Graph
        chart = VBarChart(svg, style)
        chart.draw(35, 20, data)

      # Svg Area-Chart
      with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
        style = ChartStyle()
        style.setWidth(CANVAS_WIDTH).setHeight(CANVAS_HEIGHT)
        style.setXPad(40).setPad(20)

        # Random Data
        random.seed(time() * 1000)
        data = ChartData()
        data.xaxis_label = 'X-axis'
        data.yaxis_label = 'Y-axis'
        #data.add([randint(0, 100) for i in xrange(100)], [0, 20, 10, 30, 20, 50])
        data.add(None, range(100, 200), [randint(0, 100) for i in xrange(100)])

        # Generate Graph
        chart = AreaChart(svg, style)
        chart.draw(35, 20, data)

      # Svg Pie Char
      with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
        style = ChartStyle()
        style.setWidth(CANVAS_WIDTH).setHeight(CANVAS_HEIGHT)
        style.setXPad(40).setPad(20)

        # Random Data
        data = [randint(10, 200) for i in xrange(8)]
        labels = ['Data %d = %d' % x for x in enumerate(data)]

        # Generate Graph
        chart = PieChart(svg, style)
        c = CANVAS_HEIGHT / 2
        chart.draw(c, c, data)
        chart.drawLabels(c * 2 + 5, 20, labels)

      # Svg Random Test
      with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
        svg.rectangle(50, 50, 100, 22, SvgStyle().fill('#dcdee0').stroke('#333'))
        svg.text(60, 26, 'Hello World', SvgStyle().fontSize(12).fill('#000').textAnchor('middle'))
        svg.verticalText(60, 26, 'Hello World', SvgStyle().fontSize(12).fill('#000').textAnchor('start'))
        svg.line(60, 56, 160, 26, SvgStyle().stroke('#dcdee0'))
        svg.path(SvgPath().moveTo(100, 100).curveTo(140, 110).curveTo(160, 160).close(), SvgStyle().stroke('#dcdee0').fill('none'))
      """

      def timeFormat(time):
        minutes = time / 60
        secs = time % 60
        return '%d.%d' % (minutes, secs)

      def drawLineChart(xdata, ydata):
        # Svg Line-Chart
        with svg.canvas(CANVAS_WIDTH, CANVAS_HEIGHT):
          style = ChartStyle()
          style.setWidth(CANVAS_WIDTH).setHeight(CANVAS_HEIGHT)
          style.setXPad(40).setPad(22)
          style.setXAxisFormat(timeFormat)

          # Random Data
          random.seed(time() * 1000)
          data = ChartData()
          data.xaxis_label = 'N-Reqs'
          data.yaxis_label = 'Delay'
          #data.add([randint(0, 100) for i in xrange(100)], [0, 20, 10, 30, 20, 50])
          data.add(None, xdata, ydata)

          # Generate Graph
          chart = LineChart(svg, style)
          chart.draw(35, 20, data)

      import math
      xdata = range(0, 60 * 60)

      req_sec = 100 * 1000
      drawLineChart(xdata, [int(math.sqrt(x * 100 * 1000)) for x in xdata])
      drawLineChart(xdata, [int(math.sqrt(x * 50 * 1000)) for x in xdata])
      drawLineChart(xdata, [int(math.sqrt(x * 10 * 1000)) for x in xdata])
      drawLineChart(xdata, [int(math.sqrt(x * 1000)) for x in xdata])
      drawLineChart(xdata, [int(math.sqrt(x * 500)) for x in xdata])
      drawLineChart(xdata, [int(math.log(1 + x * req_sec)) for x in xdata])

      self.wfile.write('</body>')
      self.wfile.write('</html>')
      self.wfile.flush()
      self.wfile.close()

  SocketServer.ForkingTCPServer.allow_reuse_address = True
  httpd = SocketServer.ForkingTCPServer(('', PORT), Proxy)
  httpd.allow_reuse_address = True
  print "serving at port", PORT
  try:
    while True:
      httpd.handle_request()
  finally:
    pass
