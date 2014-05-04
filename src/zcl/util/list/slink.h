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

#ifndef _Z_SLINK_H_
#define _Z_SLINK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define Z_SLINK_NODE(x)                       Z_CAST(z_slink_node_t, x)

Z_TYPEDEF_STRUCT(z_slink_node)

struct z_slink_node {
  z_slink_node_t *next;
};

#define z_slink_node(entry, type, member)     (&(((type *)(entry))->member))
#define z_slink_entry(node, type, member)     z_container_of(node, type, member)

#define z_slink_for_each(head, node, __code__)                              \
  for (node = (head)->next; node != NULL; node = (node)->next)              \
    __code__

#define z_slink_for_each_entry(head, entry, type, member, __code__)         \
  do {                                                                      \
    z_slink_node_t *__p_safe_node;                                          \
    z_slink_for_each(head, __p_safe_node, {                                 \
      entry = z_slink_entry(__p_safe_node, type, member);                   \
      do __code__ while (0);                                                \
    });                                                                     \
  } while (0)

#define z_slink_for_each_safe(head, node, __code__)                         \
  do {                                                                      \
    node = (head)->next;                                                    \
    while (node != NULL) {                                                  \
      z_slink_node_t *__p_safe_next = node->next;                           \
      do __code__ while (0);                                                \
      node = __p_safe_next;                                                 \
    }                                                                       \
  } while (0)

#define z_slink_for_each_safe_entry(head, entry, type, member, __code__)    \
  do {                                                                      \
    z_slink_node_t *__p_safe_node;                                          \
    z_slink_for_each_safe(head, __p_safe_node, {                            \
      entry = z_slink_entry(__p_safe_node, type, member);                   \
      do __code__ while (0);                                                \
    });                                                                     \
  } while (0)

__Z_END_DECLS__

#endif /* !_Z_SLINK_H_ */
