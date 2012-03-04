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

#include <zcl/streamslice.h>
#include <zcl/memory.h>
#include <zcl/stream.h>

#include "memcache_p.h"
#include "memcache.h"

#define __MEMCACHE_TABLE(object)           MEMCACHE((object)->internal->membufs)

static raleighfs_errno_t __object_create (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
    raleighfs_objdata_t *obj_data = object->internal;

    if ((obj_data->membufs = memcache_alloc(z_object_memory(fs))) == NULL)
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
    memcache_free(__MEMCACHE_TABLE(object));
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
    memcache_free(__MEMCACHE_TABLE(object));
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_query (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *msg)
{
    z_message_stream_t req_stream;
    z_message_stream_t res_stream;
    memcache_object_t *item;
    z_stream_slice_t key;
    uint32_t klength;
    uint32_t count;

    z_message_response_stream(msg, &res_stream);
    z_message_request_stream(msg, &req_stream);
    z_stream_read_uint32(Z_STREAM(&req_stream), &count);

    while (count--) {
        z_stream_read_uint32(Z_STREAM(&req_stream), &klength);
        z_stream_slice(&key, Z_STREAM(&req_stream), req_stream.offset, klength);
        z_stream_seek(Z_STREAM(&req_stream), req_stream.offset + klength);

        if ((item = memcache_lookup(__MEMCACHE_TABLE(object), Z_SLICE(&key))) == NULL) {
            z_stream_write_uint32(Z_STREAM(&res_stream), RALEIGHFS_MEMCACHE_NOT_FOUND);
            continue;
        }

        if (item->iflags & MEMCACHE_OBJECT_NUMBER)
            z_stream_write_uint32(Z_STREAM(&res_stream), RALEIGHFS_MEMCACHE_NUMBER);
        else
            z_stream_write_uint32(Z_STREAM(&res_stream), RALEIGHFS_MEMCACHE_BLOB);

        z_stream_write_uint32(Z_STREAM(&res_stream), item->exptime);
        z_stream_write_uint32(Z_STREAM(&res_stream), item->flags);
        z_stream_write_uint64(Z_STREAM(&res_stream), item->cas);

        if (item->iflags & MEMCACHE_OBJECT_NUMBER) {
            z_stream_write_uint64(Z_STREAM(&res_stream), item->data.number);
        } else {
            z_stream_write_uint32(Z_STREAM(&res_stream), item->value_size);
            z_stream_write(Z_STREAM(&res_stream), item->data.blob, item->value_size);
        }
    }

    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_insert (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    z_message_stream_t stream;
    memcache_object_t *item;
    z_stream_slice_t value;
    z_stream_slice_t key;
    uint32_t klength;
    uint32_t vlength;
    uint32_t exptime;
    uint32_t flags;
    uint64_t cas;
    int msg_type;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(Z_STREAM(&stream), &klength);
    z_stream_read_uint32(Z_STREAM(&stream), &vlength);
    z_stream_read_uint32(Z_STREAM(&stream), &flags);
    z_stream_read_uint32(Z_STREAM(&stream), &exptime);
    z_stream_read_uint64(Z_STREAM(&stream), &cas);
    z_stream_slice(&key, Z_STREAM(&stream), stream.offset, klength);
    z_stream_slice(&value, Z_STREAM(&stream), stream.offset + klength, vlength);

    /* Do item lookup */
    item = memcache_lookup(__MEMCACHE_TABLE(object), Z_SLICE(&key));
    msg_type = z_message_type(msg);

    if (msg_type == RALEIGHFS_MEMCACHE_ADD ||
        msg_type == RALEIGHFS_MEMCACHE_SET)
    {
        /*
         * SET: store this data,
         * ADD: but only if the server *doesn't* already hold data for this key.
         */
        if (item != NULL && msg_type == RALEIGHFS_MEMCACHE_ADD) {
            z_message_set_state(msg, RALEIGHFS_MEMCACHE_NOT_STORED);
            return(RALEIGHFS_ERRNO_NONE);
        }

        /* Allocate new object */
        if (item == NULL) {
            if (!(item = memcache_object_alloc(__MEMCACHE_TABLE(object)->memory,
                                               key.length)))
            {
                z_message_set_state(msg, RALEIGHFS_MEMCACHE_NO_MEMORY);
                return(RALEIGHFS_ERRNO_NO_MEMORY);
            }

            /* Set the key */
            z_slice_copy_all(&key, item->key);

            /* Add to table */
            memcache_insert(__MEMCACHE_TABLE(object), item);
            /* TODO: CHECK RET VAL */
        }

        /* Set Item Flags */
        item->exptime = exptime;
        item->flags = flags;

        /* Read Item Data */
        memcache_object_set(item, __MEMCACHE_TABLE(object)->memory, Z_SLICE(&value));
    } else if (msg_type == RALEIGHFS_MEMCACHE_CAS) {
        /* store this data but only if no one else has updated
         * since I last fetched it.
         */
        if (item == NULL) {
            z_message_set_state(msg, RALEIGHFS_MEMCACHE_NOT_STORED);
            return(RALEIGHFS_ERRNO_NONE);
        }

        if (item->cas != cas) {
            z_message_set_state(msg, RALEIGHFS_MEMCACHE_EXISTS);
            return(RALEIGHFS_ERRNO_NONE);
        }

        memcache_object_set(item, __MEMCACHE_TABLE(object)->memory, Z_SLICE(&value));
        item->cas++;
    } else if (msg_type == RALEIGHFS_MEMCACHE_REPLACE) {
        /* store this data, but only if the server *does*
         * already hold data for this key.
         */
        if (item == NULL) {
            z_message_set_state(msg, RALEIGHFS_MEMCACHE_NOT_STORED);
            return(RALEIGHFS_ERRNO_NONE);
        }

        /* Set Item Flags */
        item->exptime = exptime;
        item->flags = flags;
        item->cas++;

        /* Read Item Data */
        memcache_object_set(item, __MEMCACHE_TABLE(object)->memory, Z_SLICE(&value));
    } else if (msg_type == RALEIGHFS_MEMCACHE_APPEND) {
        /* add this data to an existing key after existing data */
        if (item == NULL) {
            z_message_set_state(msg, RALEIGHFS_MEMCACHE_NOT_STORED);
            return(RALEIGHFS_ERRNO_NONE);
        }

        item->cas++;
        memcache_object_append(item, __MEMCACHE_TABLE(object)->memory, Z_SLICE(&value));
    } else if (msg_type == RALEIGHFS_MEMCACHE_PREPEND) {
        /* add this data to an existing key before existing data */
        if (item == NULL) {
            z_message_set_state(msg, RALEIGHFS_MEMCACHE_NOT_STORED);
            return(RALEIGHFS_ERRNO_NONE);
        }

        item->cas++;
        memcache_object_prepend(item, __MEMCACHE_TABLE(object)->memory, Z_SLICE(&value));
    }

    z_message_set_state(msg, RALEIGHFS_MEMCACHE_STORED);
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_update (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    z_message_stream_t stream;
    memcache_object_t *item;
    z_stream_slice_t key;
    uint32_t klength;
    uint64_t delta;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint64(Z_STREAM(&stream), &delta);
    z_stream_read_uint32(Z_STREAM(&stream), &klength);
    z_stream_slice(&key, Z_STREAM(&stream), stream.offset, klength);

    if ((item = memcache_lookup(__MEMCACHE_TABLE(object), Z_SLICE(&key))) == NULL) {
        z_message_set_state(msg, RALEIGHFS_MEMCACHE_NOT_FOUND);
        return(RALEIGHFS_ERRNO_NONE);
    }

    if (!(item->iflags & MEMCACHE_OBJECT_NUMBER)) {
        z_message_set_state(msg, RALEIGHFS_MEMCACHE_NOT_NUMBER);
        return(RALEIGHFS_ERRNO_NONE);
    }

    switch (z_message_type(msg)) {
        case RALEIGHFS_MEMCACHE_INCR:
            item->data.number += delta;
            break;
        case RALEIGHFS_MEMCACHE_DECR:
            if (item->data.number > delta)
                item->data.number -= delta;
            else
                item->data.number = 0;
            break;
        default:
            break;
    }

    /* Fill Response */
    z_message_response_stream(msg, &stream);
    z_message_set_state(msg, RALEIGHFS_MEMCACHE_STORED);
    z_stream_write_uint64(Z_STREAM(&stream), item->data.number);

    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_remove (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    z_message_stream_t stream;
    memcache_object_t *item;
    z_stream_slice_t key;
    uint32_t klength;

    if (z_message_type(msg) == RALEIGHFS_MEMCACHE_FLUSH_ALL) {
        memcache_clear(__MEMCACHE_TABLE(object));
        return(RALEIGHFS_ERRNO_NONE);
    }

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(Z_STREAM(&stream), &klength);
    z_stream_slice(&key, Z_STREAM(&stream), stream.offset, klength);

    if ((item = memcache_lookup(__MEMCACHE_TABLE(object), Z_SLICE(&key))) == NULL) {
        z_message_set_state(msg, RALEIGHFS_MEMCACHE_NOT_FOUND);
        return(RALEIGHFS_ERRNO_NONE);
    }

    memcache_remove(__MEMCACHE_TABLE(object), item);
    z_message_set_state(msg, RALEIGHFS_MEMCACHE_DELETED);

    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_ioctl (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *msg)
{
    return(RALEIGHFS_ERRNO_NOT_IMPLEMENTED);
}

const uint8_t RALEIGHFS_OBJECT_MEMCACHE_UUID[16] = {
    80,198,43,247,80,211,70,162,134,171,222,37,252,152,233,176,
};

raleighfs_object_plug_t raleighfs_object_memcache = {
    .info = {
        .type = RALEIGHFS_PLUG_TYPE_OBJECT,
        .uuid = RALEIGHFS_OBJECT_MEMCACHE_UUID,
        .description = "Memcache Object",
        .label = "object-memcache",
    },

    .create     = __object_create,
    .open       = __object_open,
    .close      = __object_close,
    .sync       = __object_sync,
    .unlink     = __object_unlink,

    .query      = __object_query,
    .insert     = __object_insert,
    .update     = __object_update,
    .remove     = __object_remove,
    .ioctl      = __object_ioctl,
};

