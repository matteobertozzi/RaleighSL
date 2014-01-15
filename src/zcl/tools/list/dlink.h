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

#ifndef _Z_DLINK_H_
#define _Z_DLINK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define Z_DLINK_NODE(x)                       Z_CAST(z_dlink_node_t, x)

Z_TYPEDEF_STRUCT(z_dlink_node)

struct z_dlink_node {
  z_dlink_node_t *next;
  z_dlink_node_t *prev;
};

#define z_dlink_node(entry, type, member)     (&(((type *)(entry))->member))
#define z_dlink_entry(node, type, member)     z_container_of(node, type, member)

#define z_dlink_front(head)                   ((head)->next)
#define z_dlink_back(head)                    ((head)->prev)
#define z_dlink_is_empty(head)                ((head)->next == (head))
#define z_dlink_is_not_empty(head)            ((head)->next != (head))
#define z_dlink_is_linked                     z_dlink_is_empty
#define z_dlink_is_not_linked                 z_dlink_is_not_empty

#define z_dlink_front_entry(head, type, member)                         \
  z_dlink_entry(z_dlink_front(head), type, member)

#define z_dlink_back_entry(head, type, member)                          \
  z_dlink_entry(z_dlink_back(head), type, member)

#define z_dlink_init(node)                             \
  do {                                                 \
    (node)->next = node;                               \
    (node)->prev = node;                               \
  } while (0)

#define z_dlink_add(head, inew)                         \
  __z_dlink_node_add(inew, head, (head)->next)

#define z_dlink_add_tail(head, inew)                    \
  __z_dlink_node_add(inew, (head)->prev, head)

#define z_dlink_del_entry(node)                         \
  __z_dlink_node_del((node)->prev, (node)->next)

#define z_dlink_del(node)                               \
  do {                                                  \
    z_dlink_del_entry(node);                            \
    z_dlink_init(node);                                 \
  } while (0)

#define z_dlink_move(head, node)                        \
  do {                                                  \
    z_dlink_del_entry(node);                            \
    z_dlink_add(head, node);                            \
  } while (0)

#define z_dlink_move_tail(head, node)                   \
  do {                                                  \
    z_dlink_del_entry(node);                            \
    z_dlink_add_tail(head, node);                       \
  } while (0)

#define z_dlink_for_each(head, node, __code__)                              \
  for (node = (head)->next; node != (head); node = (node)->next)            \
    __code__

#define z_dlink_for_each_entry(head, entry, type, member, __code__)         \
  do {                                                                      \
    z_dlink_node_t *__p_safe_node;                                          \
    z_dlink_for_each(head, __p_safe_node, {                                 \
      entry = z_dlink_entry(__p_safe_node, type, member);                   \
      __code__                                                              \
    });                                                                     \
  } while (0)


#define z_dlink_for_each_safe(head, node, __code__)                         \
  do {                                                                      \
    z_dlink_node_t *__p_safe_next;                                          \
    for (node = (head)->next, __p_safe_next = (node)->next;                 \
         node != (head);                                                    \
         node = __p_safe_next, __p_safe_next = (node)->next)                \
      __code__                                                              \
  } while (0)

#define z_dlink_for_each_safe_entry(head, entry, type, member, __code__)    \
  do {                                                                      \
    z_dlink_node_t *__p_safe_node;                                          \
    z_dlink_for_each_safe(head, __p_safe_node, {                            \
      entry = z_dlink_entry(__p_safe_node, type, member);                   \
      __code__                                                              \
    });                                                                     \
  } while (0)

#define z_dlink_del_for_each_entry(head, entry, type, member, __code__)     \
  do {                                                                      \
    z_dlink_for_each_safe_entry(head, entry, type, member, {                \
      z_dlink_del(z_dlink_node(entry, type, member));                       \
      do __code__ while (0);                                                \
    });                                                                     \
  } while (0)

void __z_dlink_node_add (z_dlink_node_t *inew,
                         z_dlink_node_t *iprev,
                         z_dlink_node_t *inext);
void __z_dlink_node_del (z_dlink_node_t *iprev,
                         z_dlink_node_t *inext);

#endif /* !_Z_DLINK_H_ */
