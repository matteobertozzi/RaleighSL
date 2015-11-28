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

#include <zcl/dlink.h>

void __z_dlink_node_add (z_dlink_node_t *inew,
                         z_dlink_node_t *iprev,
                         z_dlink_node_t *inext)
{
  inext->prev = inew;
  inew->next = inext;
  inew->prev = iprev;
  iprev->next = inew;
}

void __z_dlink_node_del (z_dlink_node_t *iprev,
                         z_dlink_node_t *inext)
{
  inext->prev = iprev;
  iprev->next = inext;
}
