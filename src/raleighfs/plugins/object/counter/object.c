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

#include <raleighfs/execute.h>
#include <raleighfs/object.h>

#include <zcl/memory.h>
#include <zcl/stream.h>

#include "counter.h"

#define __COUNTER(object)           ((counter_t *)((object)->internal->membufs))

typedef struct counter {
    uint64_t value;
    uint64_t cas;
} counter_t;

static counter_t *__counter_alloc (z_memory_t *memory) {
    counter_t *counter;

    if ((counter = z_memory_struct_alloc(memory, counter_t)) != NULL) {
        counter->value = 0U;
        counter->cas = 0U;
    }

    return(counter);
}

static void __counter_free (z_memory_t *memory, counter_t *counter) {
    z_memory_struct_free(memory, counter_t, counter);
}


static raleighfs_errno_t __object_create (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
    raleighfs_objdata_t *obj_data = object->internal;

    if ((obj_data->membufs = __counter_alloc(z_object_memory(fs))) == NULL)
        return(RALEIGHFS_ERRNO_NO_MEMORY);

    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_open (raleighfs_t *fs,
                                        raleighfs_object_t *object)
{
    return(RALEIGHFS_ERRNO_NOT_IMPLEMENTED);
}

static raleighfs_errno_t __object_close (raleighfs_t *fs,
                                         raleighfs_object_t *object)
{
    __counter_free(z_object_memory(fs), __COUNTER(object));
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_sync (raleighfs_t *fs,
                                        raleighfs_object_t *object)
{
    return(RALEIGHFS_ERRNO_NOT_IMPLEMENTED);
}

static raleighfs_errno_t __object_unlink (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
    __counter_free(z_object_memory(fs), __COUNTER(object));
    return(RALEIGHFS_ERRNO_NONE);
}


static raleighfs_errno_t __object_query (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *msg)
{
    counter_t *counter = __COUNTER(object);
    z_message_stream_t stream;

    z_message_response_stream(msg, &stream);
    z_stream_write_uint64(Z_STREAM(&stream), counter->value);
    z_stream_write_uint64(Z_STREAM(&stream), counter->cas);

    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_update (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    counter_t *counter = __COUNTER(object);
    z_message_stream_t stream;
    uint64_t value;
    uint64_t cas;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint64(Z_STREAM(&stream), &value);

    switch (z_message_type(msg)) {
        case RALEIGHFS_COUNTER_SET:
            counter->value = value;
            counter->cas++;
            break;
        case RALEIGHFS_COUNTER_CAS:
            z_stream_read_uint64(Z_STREAM(&stream), &cas);
            if (counter->cas == cas) {
                counter->value = value;
                counter->cas++;
            }
            break;
        case RALEIGHFS_COUNTER_INCR:
            counter->value += value;
            counter->cas++;
            break;
        case RALEIGHFS_COUNTER_DECR:
            counter->value -= value;
            counter->cas++;
            break;
    }

    /* Fill response */
    z_message_response_stream(msg, &stream);
    z_stream_write_uint64(Z_STREAM(&stream), counter->value);
    z_stream_write_uint64(Z_STREAM(&stream), counter->cas);

    return(RALEIGHFS_ERRNO_NONE);
}

const uint8_t RALEIGHFS_OBJECT_COUNTER_UUID[16] = {
    79,200,57,106,240,49,72,129,155,42,43,128,199,18,183,175
};

raleighfs_object_plug_t raleighfs_object_counter = {
    .info = {
        .type = RALEIGHFS_PLUG_TYPE_OBJECT,
        .uuid = RALEIGHFS_OBJECT_COUNTER_UUID,
        .description = "Counter Object",
        .label = "object-counter",
    },

    .create     = __object_create,
    .open       = __object_open,
    .close      = __object_close,
    .sync       = __object_sync,
    .unlink     = __object_unlink,

    .query      = __object_query,
    .insert     = NULL,
    .update     = __object_update,
    .remove     = NULL,
    .ioctl      = NULL,
};

