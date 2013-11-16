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

#include <zcl/object.h>
#include <zcl/global.h>

#define Z_OBJECT_FLAGS_MEM_STACK    (1 << 0)
#define Z_OBJECT_FLAGS_MEM_HEAP     (1 << 1)

void *__z_object_alloc (z_object_t *object, const z_vtable_type_t *type, ...) {
  va_list args;

  if (object != NULL) {
    /* Object is already allocated, flag it as 'on stack' */
    object->flags.iflags = Z_OBJECT_FLAGS_MEM_STACK;
  } else {
    /* Allocate object on heap and flag it as 'on heap' */
    object = z_memory_alloc(z_global_memory(), z_object_t, type->size);
    if (Z_MALLOC_IS_NULL(object))
      return(NULL);

    object->flags.iflags = Z_OBJECT_FLAGS_MEM_HEAP;
  }

  /* Initialize Object fields */
  object->flags.u16 = 0;
  object->flags.u32 = 0;

  /* Call the object constructor */
  va_start(args, type);
  if (type->ctor(object, args)) {
    __z_object_free(object, NULL);
    object = NULL;
  }
  va_end(args);

  return(object);
}

void __z_object_free (z_object_t *object, const z_vtable_type_t *type) {
  /* Call destructor */
  if (type != NULL && type->dtor != NULL)
    type->dtor(object);

  /* Free object if it's flagged as 'on heap' */
  if (object->flags.iflags & Z_OBJECT_FLAGS_MEM_HEAP)
    z_memory_free(z_global_memory(), object);
}
