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

#ifndef _Z_BYTESLICE_H_
#define _Z_BYTESLICE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_byteslice)

struct z_byteslice {
    uint8_t *data;
    size_t length;
};

#define z_byteslice_set(slice, data_, length_)                                \
    do {                                                                      \
        (slice)->data = ((uint8_t *)data_);                                   \
        (slice)->length = length_;                                            \
    } while (0);

#define z_byteslice_clear(slice)               z_byteslice_set(slice, NULL, 0)
#define z_byteslice_is_empty(slice)            ((slice)->length == 0)

#define z_byteslice_starts_with(slice, blob, n)                               \
    (((slice)->length >= (n)) && !z_memcmp((slice)->data, blob, n))

__Z_END_DECLS__

#endif /* !_Z_BYTESLICE_H_ */
