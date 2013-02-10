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

#ifndef _Z_SLICE_H_
#define _Z_SLICE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/byteslice.h>
#include <zcl/reader.h>
#include <zcl/object.h>
#include <zcl/macros.h>
#include <zcl/string.h>

#if !defined(Z_SLICE_NBLOCKS)
    #define Z_SLICE_NBLOCKS             3
#endif

#define Z_SLICE_READER(x)               Z_CAST(z_slice_reader_t, x)
#define Z_CONST_SLICE(x)                Z_CONST_CAST(z_slice_t, x)
#define Z_SLICE(x)                      Z_CAST(z_slice_t, x)

Z_TYPEDEF_STRUCT(z_slice_interfaces)
Z_TYPEDEF_STRUCT(z_slice_reader)
Z_TYPEDEF_STRUCT(z_slice_node)
//Z_TYPEDEF_STRUCT(z_slice)

struct z_slice_interfaces {
    Z_IMPLEMENT_TYPE
    Z_IMPLEMENT_READER
};

struct z_slice_reader {
    __Z_READABLE__
    const z_slice_node_t *node;
    unsigned int iblock;
    size_t offset;
};

struct z_slice_node {
    z_slice_node_t *next;
    z_byteslice_t blocks[Z_SLICE_NBLOCKS];
};

struct z_slice {
    __Z_OBJECT__(z_slice)
    z_slice_node_t *tail;
    z_slice_node_t head;
};

z_slice_t * z_slice_alloc       (z_slice_t *self, z_memory_t *memory);
void        z_slice_free        (z_slice_t *self);

void        z_slice_clear       (z_slice_t *self);
int         z_slice_add         (z_slice_t *self, const void *data, size_t size);

size_t      z_slice_length      (const z_slice_t *self);
int         z_slice_is_empty    (const z_slice_t *self);

int         z_slice_equals      (z_slice_t *self, const void *data, size_t size);
int         z_slice_starts_with (z_slice_t *self, const void *data, size_t size);

size_t      z_slice_dump        (const z_slice_t *self);

__Z_END_DECLS__

#endif /* !_Z_SLICE_H_ */
