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

#ifndef _Z_READER_H_
#define _Z_READER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/extent.h>

typedef struct z_slice z_slice_t;

#define __Z_READABLE__                z_reader_head_t __read__;
#define Z_READER(x)                   Z_CAST(z_reader_head_t, x)
#define Z_READER_VTABLE(x)            Z_READER(x)->vtable
#define Z_READER_READABLE(type, x)    Z_CONST_CAST(type, Z_READER(x)->readable)

#define Z_IMPLEMENT_READER            Z_IMPLEMENT(reader)

#define Z_READER_INIT(self, object)                                         \
    do {                                                                    \
        Z_READER(self)->vtable = z_vtable_interface(object, reader);        \
        Z_READER(self)->readable = object;                                  \
    } while (0)

Z_TYPEDEF_STRUCT(z_vtable_reader)
Z_TYPEDEF_STRUCT(z_reader_head)

struct z_reader_head {
    const z_vtable_reader_t *vtable;
    const void *readable;
};

struct z_vtable_reader {
    int     (*open)       (void *self, const void *object);
    void    (*close)      (void *self);

    size_t  (*next)       (void *self, uint8_t **data);
    void    (*backup)     (void *self, size_t count);
    size_t  (*available)  (void *self);
};

#define z_reader_call(self, method, ...)                                  \
    Z_READER_VTABLE(self)->method(self, ##__VA_ARGS__)

#define z_reader_object(self)             Z_READER_HEAD(self).readable
#define z_reader_set_object(self, obj)    Z_READER_HEAD(self).readable = (obj)

#define z_reader_open(self, object)                                       \
    z_vtable_interface(object, reader)->open(self, object)

#define z_reader_close(self)              z_reader_call(self, close)
#define z_reader_next(self, data)         z_reader_call(self, next, data)
#define z_reader_backup(self, count)      z_reader_call(self, backup, count)
#define z_reader_available(self)          z_reader_call(self, available)

#define z_reader_tokenize(self, needle, needle_len, slice)                \
    z_v_reader_tokenize(Z_READER_VTABLE(self), self, needle, needle_len, slice)

int z_v_reader_search (const z_vtable_reader_t *vtable, void *self,
                       const void *needle, size_t needle_len,
                       z_extent_t *extent);

size_t z_v_reader_tokenize (const z_vtable_reader_t *vtable, void *self,
                            const void *needle, size_t needle_len,
                            z_slice_t *slice);

__Z_END_DECLS__

#endif /* !_Z_READER_H_ */
