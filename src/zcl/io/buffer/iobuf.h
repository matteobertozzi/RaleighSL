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

#ifndef _Z_IOBUF_H_
#define _Z_IOBUF_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/object.h>
#include <zcl/reader.h>

#define Z_IOBUF_READER(x)               Z_CAST(z_iobuf_reader_t, x)
#define Z_CONST_IOBUF(x)                Z_CONST_CAST(z_iobuf_t, x)
#define Z_IOBUF(x)                      Z_CAST(z_iobuf_t, x)

Z_TYPEDEF_STRUCT(z_iobuf_interfaces)
Z_TYPEDEF_STRUCT(z_iobuf_reader)
Z_TYPEDEF_STRUCT(z_iobuf_node)
Z_TYPEDEF_STRUCT(z_iobuf)

struct z_iobuf_interfaces {
    Z_IMPLEMENT_TYPE
    Z_IMPLEMENT_READER
};

struct z_iobuf_reader {
    __Z_READABLE__
    z_iobuf_node_t *node;
    size_t available;
    size_t offset;
};

struct z_iobuf {
    __Z_OBJECT__(z_iobuf)
    z_iobuf_node_t *head;
    z_iobuf_node_t *tail;
    unsigned int offset;
    unsigned int length;
};

z_iobuf_t *     z_iobuf_alloc         (z_iobuf_t *iobuf,
                                       z_memory_t *memory,
                                       unsigned int block);

void            z_iobuf_free          (z_iobuf_t *iobuf);

void            z_iobuf_clear         (z_iobuf_t *iobuf);
void            z_iobuf_drain         (z_iobuf_t *iobuf,
                                       unsigned int n);

ssize_t         z_iobuf_read          (z_iobuf_t *iobuf,
                                       unsigned char **data);
void            z_iobuf_read_backup   (z_iobuf_t *iobuf,
                                       ssize_t n);

ssize_t         z_iobuf_write         (z_iobuf_t *iobuf,
                                       unsigned char **data);
void            z_iobuf_write_backup  (z_iobuf_t *iobuf, ssize_t n);

void            z_iobuf_get_reader    (z_iobuf_reader_t *reader,
                                       const z_iobuf_t *iobuf);

__Z_END_DECLS__

#endif /* !_Z_IOBUF_H_ */
