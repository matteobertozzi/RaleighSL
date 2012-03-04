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

#ifndef _Z_BYTE_SLICE_H_
#define _Z_BYTE_SLICE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/types.h>

#include <zcl/slice.h>

#define Z_CONST_PREFIX_SLICE(x)                Z_CONST_CAST(z_prefix_slice_t, x)
#define Z_CONST_BYTE_SLICE(x)                  Z_CONST_CAST(z_byte_slice_t, x)
#define Z_PREFIX_SLICE(x)                      Z_CAST(z_prefix_slice_t, x)
#define Z_BYTE_SLICE(x)                        Z_CAST(z_byte_slice_t, x)

typedef struct {
    z_slice_t __base_type__;
    const uint8_t *data;
    unsigned int size;
} z_byte_slice_t;

typedef struct {
    z_slice_t __base_type__;
    const uint8_t *prefix;
    const uint8_t *data;
    unsigned int prefix_size;
    unsigned int data_size;
} z_prefix_slice_t;

void    z_byte_slice              (z_byte_slice_t *slice,
                                   const void *data,
                                   unsigned int size);

void    z_prefix_slice            (z_prefix_slice_t *slice,
                                   const void *prefix,
                                   unsigned int prefix_size,
                                   const void *data,
                                   unsigned int data_size);


__Z_END_DECLS__

#endif /* _Z_BYTE_SLICE_H_ */

