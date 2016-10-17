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


import sys
import os

LICENSE_HEADER = """
/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
"""

HEADER = """
#ifndef _{NS}_{NAME_UPPER}_H_
#define _{NS}_{NAME_UPPER}_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

__Z_END_DECLS__

#endif /* !_{NS}_{NAME_UPPER}_H_ */
"""

SOURCE = """
#include <{NS_FULL}/{NAME_LOWER}.h>
"""

def replace(text, replace_dict):
  for k, v in replace_dict.iteritems():
      text = text.replace(k, v)
  return text

if __name__ == '__main__':
  if len(sys.argv) < 2:
    print 'usage: %s <file path> [namespace]' % sys.argv[0]

  source_path = sys.argv[1]
  namespace = sys.argv[2] if len(sys.argv) >= 3 else 'zcl'
  ns = sys.argv[3] if len(sys.argv) >= 4 else namespace[0]

  path, name = os.path.split(source_path)
  if not os.path.exists(path):
    os.mkdir(path)

  replace_dict = {'{NAME_LOWER}': name.lower(),
                  '{NAME_UPPER}': name.replace('-', '_').upper(),
                  '{NS_FULL}': namespace, '{NS}': ns.upper()}


  fd = open(source_path + '.h', 'w')
  try:
    fd.write(LICENSE_HEADER.lstrip())
    fd.write(replace(HEADER, replace_dict))
  finally:
    fd.close()

  fd = open(source_path + '.c', 'w')
  try:
    fd.write(LICENSE_HEADER.lstrip())
    fd.write(replace(SOURCE, replace_dict))
  finally:
    fd.close()
