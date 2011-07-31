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

#ifndef _Z_CHUNKQ_H_
#define _Z_CHUNKQ_H_

#include <zcl/object.h>
#include <zcl/stream.h>
#include <zcl/string.h>
#include <zcl/types.h>

#define Z_CONST_CHUNKQ(x)                           Z_CONST_CAST(z_chunkq_t, x)
#define Z_CHUNKQ(x)                                 Z_CAST(z_chunkq_t, x)

typedef int (*z_chunkq_compare_t) (const void *a, const void *b, unsigned int n);

Z_TYPEDEF_STRUCT(z_chunkq_extent)
Z_TYPEDEF_STRUCT(z_chunkq_node)
Z_TYPEDEF_STRUCT(z_chunkq_iter)
Z_TYPEDEF_STRUCT(z_chunkq)

struct z_chunkq_extent {
    z_chunkq_t *    chunkq;
    unsigned int    offset;
    unsigned int    length;
};

struct z_chunkq_node {
    z_chunkq_node_t *next;
    unsigned int     offset;
    unsigned int     size;
    uint8_t          data[1];
};

struct z_chunkq {
    Z_OBJECT_TYPE

    z_chunkq_node_t *head;
    z_chunkq_node_t *tail;

    void *           user_data;
    unsigned int     chunk;
    unsigned int     size;
};

z_chunkq_t *        z_chunkq_alloc              (z_chunkq_t *chunkq,
                                                 z_memory_t *memory,
                                                 unsigned int chunk_size);
void                z_chunkq_free               (z_chunkq_t *chunkq);

void                z_chunkq_clear              (z_chunkq_t *chunkq);

unsigned int        z_chunkq_pop                (z_chunkq_t *chunkq,
                                                 void *buffer,
                                                 unsigned int size);
unsigned int        z_chunkq_read               (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 void *buffer,
                                                 unsigned int size);
unsigned int        z_chunkq_fetch              (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 unsigned int size,
                                                 z_iopush_t fetch_func,
                                                 void *user_data);

unsigned int        z_chunkq_append             (z_chunkq_t *chunkq,
                                                 const void *buffer,
                                                 unsigned int size);
unsigned int        z_chunkq_append_fetch       (z_chunkq_t *chunkq,
                                                 z_iofetch_t fetch_func,
                                                 void *user_data);
unsigned int        z_chunkq_append_fetch_fd    (z_chunkq_t *chunkq,
                                                 int fd);

unsigned int        z_chunkq_remove_push        (z_chunkq_t *chunkq,
                                                 z_iopush_t push_func,
                                                 void *user_data);
unsigned int        z_chunkq_remove_push_fd     (z_chunkq_t *chunkq,
                                                 int fd);

unsigned int        z_chunkq_prepend            (z_chunkq_t *chunkq,
                                                 const void *buffer,
                                                 unsigned int size);

void                z_chunkq_remove             (z_chunkq_t *chunkq,
                                                 unsigned int size);

long                z_chunkq_indexof            (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 const void *needle,
                                                 unsigned int needle_len);
long                z_chunkq_startswith         (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 const void *needle,
                                                 unsigned int needle_len);

int                 z_chunkq_compare            (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 const void *mem,
                                                 unsigned int mem_size,
                                                 z_chunkq_compare_t cmp_func);
int                 z_chunkq_memcmp             (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 const void *mem,
                                                 unsigned int mem_size);

int                 z_chunkq_tokenize           (z_chunkq_t *chunkq,
                                                 z_chunkq_extent_t *extent,
                                                 unsigned int offset,
                                                 const char *tokens,
                                                 unsigned int ntokens);

int                 z_chunkq_i32                (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 unsigned int length,
                                                 int base,
                                                 int32_t *value);
int                 z_chunkq_i64                (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 unsigned int length,
                                                 int base,
                                                 int64_t *value);
int                 z_chunkq_u32                (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 unsigned int length,
                                                 int base,
                                                 uint32_t *value);
int                 z_chunkq_u64                (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 unsigned int length,
                                                 int base,
                                                 uint64_t *value);

int                 z_chunkq_string             (z_chunkq_t *chunkq,
                                                 z_string_t *string,
                                                 unsigned int offset,
                                                 unsigned int length);

int                 z_chunkq_stream             (z_stream_t *stream,
                                                 z_chunkq_t *chunkq);

int                 z_stream_write_chunkq       (z_stream_t *stream,
                                                 z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 unsigned int length);

void                z_chunkq_dump               (const  z_chunkq_extent_t *ext);

#define z_chunkq_extent_copy(dst, src)                                        \
    do {                                                                      \
        (dst)->chunkq = (src)->chunkq;                                        \
        (dst)->offset = (src)->offset;                                        \
        (dst)->length = (src)->length;                                        \
    } while (0)

#define z_chunkq_extent_set(extent, cnkq, off, len)                           \
    do {                                                                      \
        (extent)->chunkq = (cnkq);                                            \
        (extent)->offset = (off);                                             \
        (extent)->length = (len);                                             \
    } while (0)

#define z_chunkq_is_null(extent)                ((extent)->chunkq == NULL)

#endif /* !_Z_CHUNKQ_H_ */

