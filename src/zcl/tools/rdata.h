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

#ifndef _Z_RDATA_H_
#define _Z_RDATA_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memory.h>
#include <zcl/macros.h>

#define Z_RDATA(x)                      Z_CAST(z_rdata_t, x)

Z_TYPEDEF_CONST_STRUCT(z_rdata_plug)
Z_TYPEDEF_STRUCT(z_rdata)

struct z_rdata_plug {
    unsigned int  (*length)     (const z_rdata_t *data);
    unsigned int  (*read)       (const z_rdata_t *data,
                                 unsigned int offset,
                                 void *buffer,
                                 unsigned int size);
    unsigned int  (*fetch)      (const z_rdata_t *data,
                                 unsigned int offset,
                                 unsigned int size,
                                 z_iopush_t func,
                                 void *user_data);

    int           (*memcmp)     (const z_rdata_t *data1,
                                 unsigned int offset1,
                                 const z_rdata_t *data2,
                                 unsigned int offset2,
                                 unsigned int length);

    int           (*rawcmp)     (const z_rdata_t *data,
                                 unsigned int offset,
                                 const void *blob,
                                 unsigned int length);
};

struct z_rdata {
    z_rdata_plug_t *plug;

    union {
        const void *custom;
        struct {
            const unsigned char *data;
            unsigned int length;
        } blob;
    } object;
};

z_rdata_t *     z_rdata_from_plug       (z_rdata_t *rdata,
                                         z_rdata_plug_t *plug,
                                         const void *object);
z_rdata_t *     z_rdata_from_blob       (z_rdata_t *rdata,
                                         const void *data,
                                         unsigned int length);
z_rdata_t *     z_rdata_from_stream     (z_rdata_t *rdata,
                                         const void *stream);
z_rdata_t *     z_rdata_from_chunkq     (z_rdata_t *rdata,
                                         const void *chunkq);

unsigned int    z_rdata_length          (const z_rdata_t *rdata);

unsigned int    z_rdata_read            (const z_rdata_t *rdata,
                                         unsigned int offset,
                                         void *buffer,
                                         unsigned int size);
unsigned int    z_rdata_readc           (const z_rdata_t *rdata,
                                         unsigned int offset,
                                         void *buffer);

unsigned int    z_rdata_fetch           (const z_rdata_t *rdata,
                                         unsigned int offset,
                                         unsigned int size,
                                         z_iopush_t func,
                                         void *user_data);
unsigned int    z_rdata_fetch_all       (const z_rdata_t *rdata,
                                         z_iopush_t func,
                                         void *user_data);
int             z_rdata_rawcmp          (const z_rdata_t *rdata,
                                         unsigned int offset,
                                         const void *data,
                                         unsigned int length);
int             z_rdata_memcmp          (const z_rdata_t *rdata1,
                                         unsigned int offset1,
                                         const z_rdata_t *rdata2,
                                         unsigned int offset2,
                                         unsigned int length);

const void *    z_rdata_blob            (const z_rdata_t *rdata);
void *          z_rdata_to_bytes        (const z_rdata_t *rdata,
                                         z_memory_t *memory);

__Z_END_DECLS__

#endif /* !_Z_RDATA_H_ */

