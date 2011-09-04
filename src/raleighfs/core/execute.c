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

#include <raleighfs/filesystem.h>
#include <raleighfs/semantic.h>
#include <raleighfs/plugins.h>
#include <raleighfs/execute.h>
#include <raleighfs/object.h>

#include <zcl/debug.h>

#include "private.h"

static raleighfs_errno_t __raleighfs_exec_object (raleighfs_t *fs,
                                                  const z_rdata_t *object_name,
                                                  z_message_t *msg)
{
    raleighfs_object_t object;
    raleighfs_errno_t errno;
    int do_close = 1;

    if ((errno = raleighfs_semantic_open(fs, &object, object_name))) {
        Z_LOG("semantic open %u %s\n", errno, raleighfs_errno_string(errno));
        return(errno);
    }

    switch (RALEIGHFS_MSG_TYPE(z_message_type(msg))) {
        case RALEIGHFS_QUERY:
            //printf("raleighfs query...\n");
            errno = raleighfs_object_query(fs, &object, msg);
            break;
        case RALEIGHFS_INSERT:
            //printf("raleighfs insert...\n");
            errno = raleighfs_object_insert(fs, &object, msg);
            break;
        case RALEIGHFS_UPDATE:
            //printf("raleighfs update...\n");
            errno = raleighfs_object_update(fs, &object, msg);
            break;
        case RALEIGHFS_REMOVE:
            //printf("raleighfs remove...\n");
            errno = raleighfs_object_remove(fs, &object, msg);
            break;
        case RALEIGHFS_IOCTL:
            Z_LOG("raleighfs ioctl...\n");
            errno = raleighfs_object_ioctl(fs, &object, msg);
            break;
        case RALEIGHFS_SYNC:
            Z_LOG("raleighfs sync...\n");
            errno = raleighfs_semantic_sync(fs, &object);
            z_message_set_state(msg, errno);
            break;
        case RALEIGHFS_UNLINK:
            Z_LOG("raleighfs unlink...\n");
            errno = raleighfs_semantic_unlink(fs, &object);
            z_message_set_state(msg, errno);
            do_close = (errno != 0);
            break;
        default:
            Z_LOG("raleighfs object unknown operation... %d\n",
                  RALEIGHFS_MSG_TYPE(z_message_type(msg)));
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    if (do_close)
        raleighfs_semantic_close(fs, &object);

    return(errno);
}

static raleighfs_errno_t __exec_semantic_create (raleighfs_t *fs,
                                                 const z_rdata_t *name,
                                                 z_message_t *msg)
{
    raleighfs_object_plug_t *plug;
    z_stream_t stream;
    uint8_t uuid[16];

    z_message_request_stream(msg, &stream);
    z_stream_read(&stream, uuid, 16);

    if ((plug = __object_plugin_lookup(fs, uuid)) == NULL)
        return(RALEIGHFS_ERRNO_PLUGIN_NOT_LOADED);

    return(raleighfs_semantic_touch(fs, plug, name));
}

static raleighfs_errno_t __exec_semantic_rename (raleighfs_t *fs,
                                                 const z_rdata_t *name,
                                                 z_message_t *msg)
{
    z_stream_extent_t extent;
    z_rdata_t new_name;
    z_stream_t stream;
    uint32_t length;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(&stream, &length);
    z_stream_set_extent(&stream, &extent, 4, length);

    z_rdata_from_stream(&new_name, &extent);
    return(raleighfs_semantic_rename(fs, name, &new_name));
}

static raleighfs_errno_t __raleighfs_exec_semantic (raleighfs_t *fs,
                                                    const z_rdata_t *obj_name,
                                                    z_message_t *msg)
{
    raleighfs_errno_t errno;

    switch (RALEIGHFS_MSG_TYPE(z_message_type(msg))) {
        case RALEIGHFS_CREATE:
            Z_LOG("raleighfs create...\n");
            errno = __exec_semantic_create(fs, obj_name, msg);
            break;
        case RALEIGHFS_RENAME:
            Z_LOG("raleighfs rename...\n");
            errno = __exec_semantic_rename(fs, obj_name, msg);
            break;
        case RALEIGHFS_EXISTS:
            Z_LOG("raleighfs exists...\n");
            errno = raleighfs_semantic_exists(fs, obj_name);
            break;
        default:
            Z_LOG("raleighfs semantic unknown operation...\n");
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    z_message_set_state(msg, errno);
    return(errno);
}

raleighfs_errno_t raleighfs_execute (raleighfs_t *fs,
                                     const z_rdata_t *object_name,
                                     z_message_t *msg)
{
    raleighfs_errno_t errno;

    if (RALEIGHFS_OBJECT_MSG(z_message_type(msg)))
        errno = __raleighfs_exec_object(fs, object_name, msg);
    else
        errno = __raleighfs_exec_semantic(fs, object_name, msg);

    return(errno);
}

