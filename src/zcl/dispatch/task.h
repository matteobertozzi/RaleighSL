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

#ifndef _Z_TASK_H_
#define _Z_TASK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/vtask.h>

Z_TYPEDEF_ENUM(z_task_rstate)
Z_TYPEDEF_STRUCT(z_task)

typedef z_task_rstate_t (*z_task_func_t) (z_task_t *task);

enum z_task_rstate {
  Z_TASK_COMPLETED,
  Z_TASK_ABORTED,
  Z_TASK_CONTINUATION,
  Z_TASK_CONTINUATION_FUNC,
  Z_TASK_YIELD,
};

struct z_task {
  z_vtask_t vtask;
  z_task_func_t func;
};

#define z_task_reset(self, ufunc)                             \
  do {                                                        \
    z_vtask_reset(&((self)->vtask), Z_VTASK_TYPE_TASK);       \
    (self)->func = ufunc;                                     \
  } while (0)

#define z_task_exec(self)         (self)->func(self)

__Z_END_DECLS__

#endif /* !_Z_TASK_H_ */
