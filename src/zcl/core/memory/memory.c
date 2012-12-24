/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#include <zcl/memory.h>
#include <zcl/string.h>

z_memory_t *z_memory_init (z_memory_t *memory,
                           const z_allocator_t *allocator,
                           void *user_data)
{
    memory->allocator = allocator;
    memory->data = user_data;
    return(memory);
}

void *__z_memory_dup (z_memory_t *memory, const void *src, unsigned int size) {
    void *dst;

    if ((dst = z_memory_raw_alloc(memory, size)) != NULL) {
        z_memcpy(dst, src, size);
    }

    return(dst);
}

void *__z_memory_zero (void *data, unsigned int size) {
    if (data != NULL)
        z_memzero(data, size);
    return(data);
}
