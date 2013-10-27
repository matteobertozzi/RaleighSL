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

#ifndef _Z_OBJECT_H_
#define _Z_OBJECT_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memory.h>
#include <zcl/atomic.h>
#include <zcl/type.h>

Z_TYPEDEF_STRUCT(z_object)

struct z_object {
  struct obj_flags {
    uint16_t iflags;        /* Internal flags */
    uint16_t uflags16;      /* User flags 16bit (free extra space) */
    uint32_t irefs;         /* Internal Reference Count */
  } flags;
};

#define __Z_OBJECT__(type)                                                    \
  z_object_t __object__;                                                      \
  const type ## _interfaces_t *vtables;

#define Z_OBJECT(x)                     Z_CAST(z_object_t, x)

#define z_object_alloc(obj, interfaces, ...)                                  \
  (obj = __z_object_alloc(Z_OBJECT(obj), (interfaces)->type, ##__VA_ARGS__),  \
   Z_LIKELY((obj) != NULL) ? ((obj)->vtables = interfaces), obj : obj)

#define z_object_free(obj)                                                    \
  __z_object_free(Z_OBJECT(obj), z_vtable_type(obj))

#define z_object_inc_ref(obj)       z_atomic_inc(&((obj)->flags.irefs))
#define z_object_dec_ref(obj)       z_atomic_dec(&((obj)->flags.irefs))

void *__z_object_alloc  (z_object_t *object, const z_vtable_type_t *type, ...);
void  __z_object_free   (z_object_t *object, const z_vtable_type_t *type);

__Z_END_DECLS__

#endif /* _Z_OBJECT_H_ */
