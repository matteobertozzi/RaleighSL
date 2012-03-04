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

#ifndef _Z_MESSAGEQ_H_
#define _Z_MESSAGEQ_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/stream.h>
#include <zcl/chunkq.h>
#include <zcl/types.h>
#include <zcl/slice.h>

Z_TYPEDEF_STRUCT(z_message_source)
Z_TYPEDEF_STRUCT(z_message_stream)
Z_TYPEDEF_STRUCT(z_messageq_plug)
Z_TYPEDEF_STRUCT(z_messageq)
Z_TYPEDEF_STRUCT(z_message)

#define Z_MESSAGE_SOURCE(x)                     Z_CAST(z_message_source_t, x)
#define Z_MESSAGE_STREAM(x)                     Z_CAST(z_message_stream_t, x)
#define Z_MESSAGEQ(x)                           Z_CAST(z_messageq_t, x)
#define Z_MESSAGE(x)                            Z_CAST(z_message_t, x)

typedef void (*z_message_func_t) (void *user_data, z_message_t *message);

typedef enum z_message_flags {
    Z_MESSAGE_ORDERED       =  1,
    Z_MESSAGE_UNORDERED     =  2,

    Z_MESSAGE_IS_READ       =  4,
    Z_MESSAGE_IS_WRITE      =  8,

    Z_MESSAGE_BYPASS        = 16,
    Z_MESSAGE_REQUEST       = 32,

    Z_MESSAGE_HAS_ERRORS    = 64,

    Z_MESSAGE_READ_REQUEST  = Z_MESSAGE_REQUEST | Z_MESSAGE_IS_READ,
    Z_MESSAGE_WRITE_REQUEST = Z_MESSAGE_REQUEST | Z_MESSAGE_IS_WRITE,
} z_message_flags_t;

#define z_message_has_flag(msg, flag) (z_message_flags(msg) & (flag))

#define z_message_is_read(msg)     z_message_has_flag(msg, Z_MESSAGE_IS_READ)
#define z_message_is_write(msg)    z_message_has_flag(msg, Z_MESSAGE_IS_WRITE)
#define z_message_is_bypass(msg)   z_message_has_flag(msg, Z_MESSAGE_BYPASS)
#define z_message_is_request(msg)  z_message_has_flag(msg, Z_MESSAGE_REQUEST)
#define z_message_has_errors(msg)  z_message_has_flag(msg, Z_MESSAGE_HAS_ERRORS)

struct z_messageq_plug {
    int     (*init)     (z_messageq_t *messageq);
    void    (*uninit)   (z_messageq_t *messageq);
    int     (*send)     (z_messageq_t *messageq, 
                         const z_slice_t *object,
                         z_message_t *message);
    void    (*yield)    (z_messageq_t *messageq, 
                         z_message_t *message);
};

struct z_message_stream {
    z_stream_t __base_type__;
    z_message_t *message;
    z_chunkq_t *chunkq;
    unsigned int offset;
};

struct z_messageq {
    Z_OBJECT_TYPE

    z_messageq_plug_t *plug;
    z_data_t           plug_data;

    /* Executor function */
    z_message_func_t   exec_func;
    void *             user_data;
};

extern z_messageq_plug_t z_messageq_noop;
extern z_messageq_plug_t z_messageq_local;

/* MessageQ related */
z_messageq_t *      z_messageq_alloc            (z_messageq_t *messageq,
                                                 z_memory_t *memory,
                                                 z_messageq_plug_t *plug,
                                                 z_message_func_t exec_func,
                                                 void *user_data);
void                z_messageq_free             (z_messageq_t *messageq);

/* Message Source related */
z_message_source_t *z_message_source_alloc      (z_messageq_t *messageq);
void                z_message_source_free       (z_message_source_t *source);

z_messageq_t *      z_message_source_queue      (z_message_source_t *source);

/* Message related */
z_message_t *       z_message_alloc             (z_message_source_t *source,
                                                 unsigned int flags);
void                z_message_free              (z_message_t *message);

int                 z_message_send              (z_message_t *message,
                                                 const z_slice_t *object_name,
                                                 z_message_func_t callback,
                                                 void *user_data);
void                z_message_yield             (z_message_t *message);
void                z_message_yield_sub_task    (z_message_t *message,
                                                 z_message_func_t func,
                                                 void *data);

void *              z_message_sub_task_data     (z_message_t *message);
void                z_message_set_sub_task      (z_message_t *message,
                                                 z_message_func_t func,
                                                 void *data);
void                z_message_unset_sub_task    (z_message_t *message);

const z_slice_t *   z_message_object            (z_message_t *message);
z_message_source_t *z_message_source            (z_message_t *message);
z_messageq_t *      z_message_queue             (z_message_t *message);
unsigned int        z_message_flags             (z_message_t *message);

unsigned int        z_message_type              (z_message_t *message);
void                z_message_set_type          (z_message_t *message,
                                                 unsigned int type);

unsigned int        z_message_state             (z_message_t *message);
void                z_message_set_state         (z_message_t *message,
                                                 unsigned int state);

int                 z_message_request_stream    (z_message_t *message,
                                                 z_message_stream_t *stream);
int                 z_message_response_stream   (z_message_t *message,
                                                 z_message_stream_t *stream);

__Z_END_DECLS__

#endif /* !_Z_MESSAGEQ_H_ */

