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

#ifndef _Z_TYPE_H_
#define _Z_TYPE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_type_interfaces);
Z_TYPEDEF_STRUCT(z_vtable_type);

#define Z_IMPLEMENT(name)                   const z_vtable_ ## name ## _t *name;
#define Z_IMPLEMENT_TYPE                    Z_IMPLEMENT(type)

typedef int  (*z_type_ctor_t) (void *self, va_list args);
typedef void (*z_type_dtor_t) (void *self);

struct z_vtable_type {
  const char *  name;
  unsigned int  size;

  z_type_ctor_t ctor;
  z_type_dtor_t dtor;
};

struct z_type_interfaces {
  Z_IMPLEMENT_TYPE
};

#define z_vtable_interface(obj, interface)      ((obj)->vtables->interface)
#define z_vtable_type(obj)                      z_vtable_interface(obj, type)

#define z_type_name(self)                       (z_vtable_type(self)->name)
#define z_type_size(self)                       (z_vtable_type(self)->size)

#define z_vtable_method(obj, type, method)                                    \
  z_vtable_interface(obj, type)->method

#define z_vtable_has_method(obj, type, method)                                \
  (z_vtable_interface(obj, type, method) != NULL)

#define z_vtable_call(obj, type, method, ...)                                 \
  z_vtable_method(obj, type, method)(obj, ##__VA_ARGS__)

int   z_type_noop_ctor (void *self, va_list args);
void  z_type_noop_dtor (void *self);

__Z_END_DECLS__

#endif /* _Z_TYPE_H_ */
