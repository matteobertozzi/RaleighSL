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

#ifndef _Z_BYTEARRAY_H_
#define _Z_BYTEARRAY_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/object.h>

#define Z_CONST_BYTES(x)                Z_CONST_CAST(z_bytes_t, x)
#define Z_BYTES(x)                      Z_CAST(z_bytes_t, x)

Z_TYPEDEF_STRUCT(z_bytes_interfaces)
Z_TYPEDEF_STRUCT(z_bytes)

struct z_bytes_interfaces {
    Z_IMPLEMENT_TYPE
};

struct z_bytes {
    __Z_OBJECT__(z_bytes)
    uint8_t *block;
    size_t blksize;
    size_t size;
};

z_bytes_t * z_bytes_alloc       (z_bytes_t *self, z_memory_t *memory);
void        z_bytes_free        (z_bytes_t *self);


int         z_bytes_squeeze     (z_bytes_t *self);
int         z_bytes_reserve     (z_bytes_t *self, size_t reserve);

int         z_bytes_clear       (z_bytes_t *self);
int         z_bytes_truncate    (z_bytes_t *self, size_t size);
int         z_bytes_remove      (z_bytes_t *self, size_t index, size_t size);

int         z_bytes_set         (z_bytes_t *self, const void *blob, size_t size);
int         z_bytes_append      (z_bytes_t *self, const void *blob, size_t size);
int         z_bytes_prepend     (z_bytes_t *self, const void *blob, size_t size);
int         z_bytes_insert      (z_bytes_t *self,
                                 size_t index,
                                 const void *blob,
                                 size_t size);
int         z_bytes_replace     (z_bytes_t *self,
                                 size_t index,
                                 size_t size,
                                 const void *blob,
                                 size_t n);

int         z_bytes_equal       (z_bytes_t *self, const void *blob, size_t size);
int         z_bytes_compare     (z_bytes_t *self, const void *blob, size_t size);

__Z_END_DECLS__

#endif /* !_Z_BYTEARRAY_H_ */
