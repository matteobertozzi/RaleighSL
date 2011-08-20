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

#ifndef _Z_BUFFER_H_
#define _Z_BUFFER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/stream.h>
#include <zcl/types.h>

#define Z_CONST_BUFFER(x)                       Z_CONST_CAST(z_buffer_t, x)
#define Z_BUFFER(x)                             Z_CAST(z_buffer_t, x)

Z_TYPEDEF_STRUCT(z_buffer)

struct z_buffer {
    Z_OBJECT_TYPE

    z_mem_grow_t    grow_policy;
    void *          user_data;
    unsigned char * blob;
    unsigned int    block;
    unsigned int    size;
};

z_buffer_t *      z_buffer_alloc            (z_buffer_t *buffer,
                                             z_memory_t *memory,
                                             z_mem_grow_t grow_policy,
                                             void *user_data);
void              z_buffer_free             (z_buffer_t *buffer);

int               z_buffer_squeeze          (z_buffer_t *buffer);
int               z_buffer_reserve          (z_buffer_t *buffer,
                                             unsigned int reserve);

int               z_buffer_clear            (z_buffer_t *buffer);
int               z_buffer_truncate         (z_buffer_t *buffer,
                                             unsigned int size);
int               z_buffer_remove           (z_buffer_t *buffer,
                                             unsigned int index,
                                             unsigned int size);

int               z_buffer_set              (z_buffer_t *buffer,
                                             const void *blob,
                                             unsigned int size);
int               z_buffer_append           (z_buffer_t *buffer,
                                             const void *blob,
                                             unsigned int size);
int               z_buffer_prepend          (z_buffer_t *buffer,
                                             const void *blob,
                                             unsigned int size);
int               z_buffer_insert           (z_buffer_t *buffer,
                                             unsigned int index,
                                             const void *blob,
                                             unsigned int size);

int               z_buffer_replace          (z_buffer_t *buffer,
                                             unsigned int index,
                                             unsigned int size,
                                             const void *blob,
                                             unsigned int n);

int               z_buffer_equal            (z_buffer_t *buffer,
                                             const void *blob,
                                             unsigned int size);
int               z_buffer_compare          (z_buffer_t *buffer,
                                             const void *blob,
                                             unsigned int size);

int               z_buffer_stream           (z_stream_t *stream,
                                             z_buffer_t *buffer);

__Z_END_DECLS__

#endif /* !_Z_BUFFER_H_ */

