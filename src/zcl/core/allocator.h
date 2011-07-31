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

#ifndef _Z_ALLOCATOR_H_
#define _Z_ALLOCATOR_H_

#include <stdint.h>
#include <stdlib.h>

#include <zcl/macros.h>

#define Z_ALLOCATOR(x)              Z_CAST(z_allocator_t, x)

Z_TYPEDEF_STRUCT(z_allocator)

typedef void *       (*z_mem_alloc_t)       (void *user_data,
                                             unsigned int size);
typedef void *       (*z_mem_realloc_t)     (void *user_data,
                                             void *ptr,
                                             unsigned int size);
typedef void         (*z_mem_free_t)        (void *user_data,
                                             void *ptr);
typedef unsigned int (*z_mem_grow_t)        (void *user_data,
                                             unsigned int size);

struct z_allocator {
    z_mem_alloc_t   alloc;
    z_mem_realloc_t realloc;
    z_mem_free_t    free;

    void *          data;
};

z_allocator_t * z_system_allocator  (void);

#endif /* !_Z_ALLOCATOR_H_ */

