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

#ifndef _Z_FREE_LIST_H_
#define _Z_FREE_LIST_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_free_list_node z_free_list_node_t;
typedef struct z_free_list z_free_list_t;

#define Z_FREE_LIST_NODE(x)       Z_CAST(z_free_list_node_t, x)
#define Z_FREE_LIST(x)            Z_CAST(z_free_list_t, x)

struct z_free_list {
  z_free_list_node_t *head;
};

#define   z_free_list_init(self)  (self)->head = NULL
void      z_free_list_add         (z_free_list_t *self,
                                   void *memory);
void      z_free_list_add_sized   (z_free_list_t *self,
                                   void *memory,
                                   size_t size);
void *    z_free_list_pop         (z_free_list_t *self);
void *    z_free_list_pop_fit     (z_free_list_t *self,
                                   size_t required,
                                   size_t *size);
void *    z_free_list_pop_best    (z_free_list_t *self,
                                   size_t required,
                                   size_t *size);

__Z_END_DECLS__

#endif /* !_Z_FREE_LIST_H_ */
