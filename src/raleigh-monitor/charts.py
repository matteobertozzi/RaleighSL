#!/usr/bin/env python
#
#   Copyright 2013 Matteo Bertozzi
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

import json

class PieChart(object):
  def __init__(self, title, height):
    self.title = title
    self.height = height
    self.values = []

  def add(self, label, value):
    self.values.append((label, value))

  def toJsonData(self):
    # [{ key: "Title", values: [ {"label": "A", "value" : 1} ]}]
    values = [json.dumps(dict(label=k, value=v)) for k, v in self.values]
    return '[{key: %s, values: [%s]}]' % (json.dumps(self.title), ', '.join(values))

class _SeriesChart(object):
  def __init__(self, title, height):
    self.title = title
    self.height = height
    self.series = []

  def add(self, label, values):
    self.series.append((label, values))

  def toJsonData(self):
    # {"key": "Series 4", "values": [ [ 1025409600000 , -7.0674410638835] ]}
    series = []
    for label, values in self.series:
      series.append('{"key": %s, "values": %s}' % (json.dumps(label), json.dumps(values)))
    return '[%s]' % (', '.join(series))

class StackedAreaChart(_SeriesChart):
  pass

class CumulativeLineChart(_SeriesChart):
  pass

class MultiBarChart(_SeriesChart):
  pass
