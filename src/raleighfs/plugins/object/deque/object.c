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

#include "deque_p.h"
#include "deque.h"

#define __DEQUE(object)           ((deque_t *)((object)->internal->membufs))

static raleighfs_errno_t __object_create (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
    deque_t *deque;

    if ((deque = deque_alloc(z_object_memory(fs))) == NULL)
        return(RALEIGHFS_ERRNO_NO_MEMORY);

    object->internal->membufs = deque;

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
    deque_free(z_object_memory(fs), __DEQUE(object));
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
    deque_free(z_object_memory(fs), __DEQUE(object));
    return(RALEIGHFS_ERRNO_NONE);
}


static void __serialize_deque_object (z_stream_t *stream,
                                      deque_object_t *dobject)
{
    if (dobject != NULL) {
        z_stream_write_uint32(stream, dobject->length);
        z_stream_write(stream, dobject->data, dobject->length);
    } else {
        z_stream_write_uint32(stream, 0);
    }
}

static raleighfs_errno_t __object_query (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *msg)
{
    deque_object_t *dobj;
    z_stream_t stream;

    z_message_response_stream(msg, &stream);

    switch (z_message_type(msg)) {
        case RALEIGHFS_DEQUE_GET_BACK:
            dobj = deque_get_back(__DEQUE(object));
            __serialize_deque_object(&stream, dobj);
            break;
        case RALEIGHFS_DEQUE_GET_FRONT:
            dobj = deque_get_front(__DEQUE(object));
            __serialize_deque_object(&stream, dobj);
            break;
        case RALEIGHFS_DEQUE_LENGTH:
            z_stream_write_uint64(&stream, __DEQUE(object)->length);
            break;
        case RALEIGHFS_DEQUE_STATS:
            break;
    }

    z_message_set_state(msg, RALEIGHFS_ERRNO_NONE);
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_insert (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    z_stream_extent_t value;
    raleighfs_errno_t errno;
    z_stream_t stream;
    uint32_t vlength;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(&stream, &vlength);
    z_stream_set_extent(&stream, &value, 4, vlength);

    switch (z_message_type(msg)) {
        case RALEIGHFS_DEQUE_PUSH_BACK:
            errno = deque_push_back(__DEQUE(object), &value);
            break;
        case RALEIGHFS_DEQUE_PUSH_FRONT:
            errno = deque_push_front(__DEQUE(object), &value);
            break;
        default:
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    z_message_set_state(msg, errno);
    return(errno);
}

static raleighfs_errno_t __object_remove (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    deque_object_t *dobj;
    z_stream_t stream;

    z_message_response_stream(msg, &stream);

    switch (z_message_type(msg)) {
        case RALEIGHFS_DEQUE_RM_BACK:
            deque_remove_back(__DEQUE(object));
            break;
        case RALEIGHFS_DEQUE_RM_FRONT:
            deque_remove_front(__DEQUE(object));
            break;
        case RALEIGHFS_DEQUE_POP_FRONT:
            dobj = deque_pop_front(__DEQUE(object));
            __serialize_deque_object(&stream, dobj);
            if (dobj != NULL) deque_object_free(z_object_memory(fs), dobj);
            break;
        case RALEIGHFS_DEQUE_POP_BACK:
            dobj = deque_pop_back(__DEQUE(object));
            __serialize_deque_object(&stream, dobj);
            if (dobj != NULL) deque_object_free(z_object_memory(fs), dobj);
            break;
        case RALEIGHFS_DEQUE_CLEAR:
            deque_clear(__DEQUE(object));
            break;
    }

    z_message_set_state(msg, RALEIGHFS_ERRNO_NONE);
    return(RALEIGHFS_ERRNO_NONE);
}

const uint8_t RALEIGHFS_OBJECT_DEQUE_UUID[16] = {
    194,166,44,241,88,191,71,187,191,209,158,213,104,169,247,242
};

raleighfs_object_plug_t raleighfs_object_deque = {
    .info = {
        .type = RALEIGHFS_PLUG_TYPE_OBJECT,
        .uuid = RALEIGHFS_OBJECT_DEQUE_UUID,
        .description = "Deque Object",
        .label = "object-deque",
    },

    .create     = __object_create,
    .open       = __object_open,
    .close      = __object_close,
    .sync       = __object_sync,
    .unlink     = __object_unlink,

    .query      = __object_query,
    .insert     = __object_insert,
    .update     = NULL,
    .remove     = __object_remove,
    .ioctl      = NULL,
};
