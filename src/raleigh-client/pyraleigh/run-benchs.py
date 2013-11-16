#!/usr/bin/env python
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

import imp
import os

import sys

BENCHMARK_DIR = os.path.join(os.path.split(os.path.abspath(__file__))[0], 'benchmark')
sys.path.append(BENCHMARK_DIR)

def load_from_file(filepath):
  mod_name,file_ext = os.path.splitext(os.path.split(filepath)[-1])
  if file_ext.lower() == '.py':
    module = imp.load_source(mod_name, filepath)
  elif file_ext.lower() == '.pyc':
    module = imp.load_compiled(mod_name, filepath)
  return module

if __name__ == '__main__':
  for root, dirs, files in os.walk('benchmark', topdown=False):
    for name in files:
      if name.startswith('bench_'):
        module = load_from_file(os.path.join(root, name))
        getattr(module, 'main')()
