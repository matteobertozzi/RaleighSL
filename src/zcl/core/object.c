/*
 *   Copyright 2011 Matteo Bertozzi
 *
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

void *__z_object_alloc (z_memory_t *memory,
                        z_object_t *object,
                        unsigned short int type,
                        unsigned int size)
{
    if (object != NULL) {
        /* Object is already allocated, flag it as 'on stack' */
        object->flags.base_flags = Z_OBJECT_FLAGS_MEM_STACK;
    } else {
        /* Allocate object on heap and flag it as 'on heap' */
        if ((object = z_memory_block_alloc(memory, z_object_t, size)) == NULL)
            return(NULL);

        object->flags.base_flags = Z_OBJECT_FLAGS_MEM_HEAP;
    }

    /* Initialize Object Fields */
    object->memory = memory;
    object->flags.type = type;
    object->flags.unused = 0;
    object->flags.user_flags = 0;

    return(object);
}

void __z_object_free (z_object_t *object) {
    /* Free object if it's flagged as 'on heap' */
    if (object->flags.base_flags & Z_OBJECT_FLAGS_MEM_HEAP)
        z_memory_free(object->memory, object);
}

