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

#ifndef _Z_EXTENT_H_
#define _Z_EXTENT_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_extent)

struct z_extent {
    size_t offset;
    size_t length;
};

#define z_extent_set(extent, voffset, vlength)                              \
    do {                                                                    \
        (extent)->offset = voffset;                                         \
        (extent)->length = vlength;                                         \
    } while (0)

#define z_extent_reset(extent)             z_extent_set(extent, 0, 0)

#endif /* !_Z_EXTENT_H_ */
