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
