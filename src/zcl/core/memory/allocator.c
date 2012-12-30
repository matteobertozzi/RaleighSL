/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

#include <stdlib.h>

#include <zcl/allocator.h>

static void *__system_alloc (void *user_data, unsigned int size) {
    return(malloc(size));
}

static void *__system_realloc (void *user_data, void *ptr, unsigned int size) {
    return(realloc(ptr, size));
}

static void __system_free (void *user_data, void *ptr) {
    free(ptr);
}

static const z_allocator_t __system_allocator = {
    .alloc   = __system_alloc,
    .realloc = __system_realloc,
    .free    = __system_free,
};

const z_allocator_t *z_system_allocator (void) {
    return(&__system_allocator);
}
