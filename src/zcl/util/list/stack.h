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

#ifndef _Z_STACK_H_
#define _Z_STACK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/slink.h>

#define Z_STACK(x)                         Z_CAST(z_stack, x)

Z_TYPEDEF_STRUCT(z_stack)

struct z_stack {
  z_slink_node_t *head;
};

#define z_stack_head(stack)               ((stack)->head)
#define z_stack_is_empty(stack)           ((stack)->head == NULL)
#define z_stack_is_not_empty(stack)       ((stack)->head != NULL)

#define z_stack_entry_head(stack, type, member)             \
  z_container_of((stack)->head, type, member)

#define z_stack_init(stack)                                 \
  do {                                                      \
    (stack)->head = NULL;                                   \
  } while (0)

#define z_stack_add(stack, node)                            \
  do {                                                      \
    (node)->next = (stack)->head;                           \
    (stack)->head = node;                                   \
  } while (0)

#define z_stack_remove(stack)                               \
  do {                                                      \
    (stack)->head = (stack)->head->next;                    \
  } while (0)

#define z_stack_for_each(stack, node, __code__)                             \
  z_slink_for_each((stack)->head, node, __code__)

#define z_stack_for_each_entry(stack, entry, type, member, __code__)        \
  z_slink_for_each_entry((stack)->head, entry, type, member, __code__)

#define z_stack_for_each_safe(stack, node, __code__)                        \
  z_slink_for_each_safe((stack)->head, node, __code__)

#define z_stack_for_each_safe_entry(stack, entry, type, member, __code__)   \
  z_slink_for_each_safe_entry((stack)->head, entry, type, member, __code__)

__Z_END_DECLS__

#endif /* !_Z_SLINK_H_ */
