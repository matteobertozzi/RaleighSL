/*
 *   Copyright 2012 Matteo Bertozzi
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

#include <zcl/macros.h>
#include <zcl/types.h>

#include <zcl/stream.h>

#define Z_CONST_SLICE(x)                    Z_CONST_CAST(z_slice_t, x)
#define Z_SLICE(x)                          Z_CAST(z_slice_t, x)

Z_TYPEDEF_CONST_STRUCT(z_slice_vtable)
Z_TYPEDEF_STRUCT(z_slice)

struct z_slice_vtable {
    unsigned int (*length)   (const z_slice_t *slice);

    int          (*copy)     (const z_slice_t *slice,
                              unsigned int offset,
                              void *buffer,
                              unsigned int length);

    int          (*write)    (const z_slice_t *slice,
                              z_stream_t *stream);
    
    int          (*rawcmp)   (const z_slice_t *slice,
                              unsigned int offset,
                              const void *blob,
                              unsigned int length);
    
    int          (*fetch)    (const z_slice_t *data,
                              unsigned int offset,
                              unsigned int size,
                              z_iopush_t func,
                              void *user_data);
    
    uint8_t      (*fetch8)   (const z_slice_t *slice,
                              unsigned int offset);
    uint16_t     (*fetch16)  (const z_slice_t *slice,
                              unsigned int offset);
    uint32_t     (*fetch32)  (const z_slice_t *slice,
                              unsigned int offset);
    uint64_t     (*fetch64)  (const z_slice_t *slice,
                              unsigned int offset);
};

struct z_slice {
    z_slice_vtable_t *vtable;
};

#define z_slice_is_empty(slice)   (Z_VTABLE_CALL(z_slice_t, slice, length) == 0)
#define z_slice_length(slice)     Z_VTABLE_CALL(z_slice_t, slice, length)

#define z_slice_copy(slice, offset, buffer, length)                       \
    Z_VTABLE_CALL(z_slice_t, slice, copy, offset, buffer, length)

#define z_slice_copy_all(slice, buffer)                                   \
    z_slice_copy(slice, 0, buffer, z_slice_length(slice))

#define z_slice_write(slice, stream)                                      \
    Z_VTABLE_CALL(z_slice_t, slice, write, stream)

#define z_slice_fetch(slice, offset, size, func, user_data)               \
    Z_VTABLE_CALL(z_slice_t, slice, fetch, offset, size, func, user_data)

#define z_slice_fetch_all(slice, func, user_data)                         \
    z_slice_fetch(slice, 0, z_slice_length(slice), func, user_data)

#define z_slice_fetch8(slice, offset)                                     \
    Z_VTABLE_CALL(z_slice_t, slice, fetch8, offset)

#define z_slice_fetch16(slice, offset)                                    \
    (Z_VTABLE_HAS_METHOD(z_slice_t, slice, fetch16) ?                     \
        Z_VTABLE_CALL(z_slice_t, slice, fetch16, offset) :                \
        _z_slice_fetch16(slice, offset))

#define z_slice_fetch32(slice, offset)                                    \
    (Z_VTABLE_HAS_METHOD(z_slice_t, slice, fetch32) ?                     \
        Z_VTABLE_CALL(z_slice_t, slice, fetch32, offset) :                \
        _z_slice_fetch32(slice, offset))

#define z_slice_fetch64(slice, offset)                                    \
    (Z_VTABLE_HAS_METHOD(z_slice_t, slice, fetch64) ?                     \
        Z_VTABLE_CALL(z_slice_t, slice, fetch64, offset) :                \
        _z_slice_fetch64(slice, offset))

#define z_slice_rawcmp(slice, offset, data, length)                       \
    Z_VTABLE_CALL(z_slice_t, slice, rawcmp, offset, data, length)

int z_slice_compare (const z_slice_t *a, const z_slice_t *b);

uint16_t _z_slice_fetch16 (const z_slice_t *slice, unsigned int offset);
uint32_t _z_slice_fetch32 (const z_slice_t *slice, unsigned int offset);
uint64_t _z_slice_fetch64 (const z_slice_t *slice, unsigned int offset);

__Z_END_DECLS__

#endif /* !_Z_SLICE_H_ */

