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

#ifndef _Z_STAILQ_H_
#define _Z_STAILQ_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/slink.h>

#define Z_STAILQ(x)                         Z_CAST(z_stailq, x)

Z_TYPEDEF_STRUCT(z_stailq)

struct z_stailq {
  z_slink_node_t *head;
  z_slink_node_t **tail;
};

#define z_stailq_head(stailq)                ((stailq)->head)
#define z_stailq_is_empty(stailq)           ((stailq)->head == NULL)
#define z_stailq_is_not_empty(stailq)       ((stailq)->head != NULL)

#define z_stailq_entry_head(stailq, type, member)           \
  z_container_of((stailq)->head, type, member)

#define z_stailq_init(stailq)                               \
  do {                                                      \
    (stailq)->head = NULL;                                  \
    (stailq)->tail = &((stailq)->head);                     \
  } while (0)

#define z_stailq_add(stailq, node)                          \
  do {                                                      \
    (node)->next = NULL;                                    \
    *((stailq)->tail) = node;                               \
    (stailq)->tail = &((node)->next);                       \
  } while (0)

#define z_stailq_remove(stailq)                             \
  do {                                                      \
    (stailq)->head = (stailq)->head->next;                  \
    if (Z_UNLIKELY((stailq)->head == NULL)) {               \
      (stailq)->tail = &((stailq)->head);                   \
    }                                                       \
  } while (0)

#define z_stailq_for_each(stailq, node, __code__)                             \
  for (node = (stailq)->head; node != NULL; node = (node)->next)              \
    __code__

#define z_stailq_for_each_entry(stailq, entry, type, member, __code__)        \
  do {                                                                        \
    z_slink_node_t *__p_safe_node;                                            \
    z_stailq_for_each(stailq, __p_safe_node, {                                \
      entry = z_slink_entry(__p_safe_node, type, member);                     \
      do __code__ while (0);                                                  \
    });                                                                       \
  } while (0)

#define z_stailq_for_each_safe(stailq, node, __code__)                        \
  do {                                                                        \
    node = (stailq)->head;                                                    \
    while (node != NULL) {                                                    \
      z_slink_node_t *__p_safe_next = node->next;                             \
      do __code__ while (0);                                                  \
      node = __p_safe_next;                                                   \
    }                                                                         \
  } while (0)

#define z_stailq_for_each_safe_entry(stailq, entry, type, member, __code__)   \
  do {                                                                        \
    z_slink_node_t *__p_safe_node;                                            \
    z_stailq_for_each_safe(stailq, __p_safe_node, {                           \
      entry = z_slink_entry(__p_safe_node, type, member);                     \
      do __code__ while (0);                                                  \
    });                                                                       \
  } while (0)

__Z_END_DECLS__

#endif /* !_Z_SLINK_H_ */
