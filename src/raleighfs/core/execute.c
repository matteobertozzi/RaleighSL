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

#include <zcl/thread.h>
#include <zcl/debug.h>

#include "private.h"

/* ============================================================================
 *  Read/Write Lock
 */
#define __begin_process(rw, is_read)                                    \
    ((is_read) ? __begin_read_process(rw) : __begin_write_process(rw))

/* Try to acquire a read lock */
static int __begin_read_process (raleighfs_rwlock_t *rw) {
    int can_begin;

    z_spin_lock(&(rw->lock));
    if ((can_begin = (rw->access_count >= 0 && rw->wr_waiting == 0)))
        ++rw->access_count;
    z_spin_unlock(&(rw->lock));

    return(can_begin);
}

/* Try to acquire a write lock */
static int __begin_write_process (raleighfs_rwlock_t *rw) {
    int can_begin;

    z_spin_lock(&(rw->lock));
    if ((can_begin = (rw->access_count == 0))) {
        --rw->access_count;
        rw->wr_waiting = 0;
    } else {
        rw->wr_waiting = 1;
    }
    z_spin_unlock(&(rw->lock));

    return(can_begin);
}

/* Release read or write lock */
static void __end_process (raleighfs_rwlock_t *rw) {
    z_spin_lock(&(rw->lock));
    if (rw->access_count > 0) {
        /* Releasing a read lock */
        --rw->access_count;
    } else if (rw->access_count < 0) {
        /* Releasing a write lock */
        ++rw->access_count;
    }
    z_spin_unlock(&(rw->lock));
}

/* ============================================================================
 *  Executor functions
 */
static void __task_exec_close (void *user_data, z_message_t *msg) {
    raleighfs_t *fs = RALEIGHFS(user_data);
    raleighfs_object_t object;

    /* Initialize object */
    object.internal = z_message_sub_task_data(msg);

    /* lock the semantic layer */
    if (!__begin_write_process(&(fs->lock))) {
        z_message_yield(msg);
        return;
    }

    raleighfs_semantic_close(fs, &object);

    /* unlock the semantic layer */
    __end_process(&(fs->lock));

    /* No more sub-task */
    z_message_unset_sub_task(msg);
}

static void __task_exec_object (void *user_data, z_message_t *msg) {
    raleighfs_t *fs = RALEIGHFS(user_data);
    raleighfs_objdata_t *objdata;
    raleighfs_object_t object;
    raleighfs_errno_t errno;

    /* Initialize object */
    objdata = z_message_sub_task_data(msg);
    object.internal = objdata;

    /* lock the object */
    if (!__begin_process(&(objdata->lock), z_message_is_read(msg))) {
        z_message_yield(msg);
        return;
    }

    /* execute operation */
    switch (RALEIGHFS_MSG_TYPE(z_message_type(msg))) {
        case RALEIGHFS_QUERY:
            errno = raleighfs_object_query(fs, &object, msg);
            break;
        case RALEIGHFS_INSERT:
            errno = raleighfs_object_insert(fs, &object, msg);
            break;
        case RALEIGHFS_UPDATE:
            errno = raleighfs_object_update(fs, &object, msg);
            break;
        case RALEIGHFS_REMOVE:
            errno = raleighfs_object_remove(fs, &object, msg);
            break;
        case RALEIGHFS_IOCTL:
            errno = raleighfs_object_ioctl(fs, &object, msg);
            break;
        case RALEIGHFS_SYNC:
            errno = raleighfs_semantic_sync(fs, &object);
            z_message_set_state(msg, errno);
            break;
        default:
            Z_LOG("raleighfs object unknown operation... %d\n",
                  RALEIGHFS_MSG_TYPE(z_message_type(msg)));
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    /* unlock the object */
    __end_process(&(objdata->lock));

    /* Schedule sub-task for close */
    z_message_set_sub_task(msg, __task_exec_close, objdata);
}

static void __task_exec_unlink (void *user_data, z_message_t *msg) {
    raleighfs_t *fs = RALEIGHFS(user_data);
    raleighfs_objdata_t *objdata;
    raleighfs_object_t object;
    raleighfs_errno_t errno;

    /* Initialize object */
    objdata = z_message_sub_task_data(msg);
    object.internal = objdata;

    /* Try lock the semantic layer */
    if (!__begin_write_process(&(fs->lock))) {
        z_message_yield(msg);
        return;
    }

    /* Try lock the object */
    if (!__begin_write_process(&(objdata->lock))) {
        __end_process(&(fs->lock));
        z_message_yield(msg);
        return;
    }

    /* FIXME: someone can be yielded for __task_exec_object */
    errno = raleighfs_semantic_unlink(fs, &object);    

    /* unlock the semantic layer */
    __end_process(&(fs->lock));

    /* No more sub-task */
    z_message_unset_sub_task(msg);

    /* Set state for message */    
    z_message_set_state(msg, errno);
}

static void __raleighfs_exec_open (raleighfs_t *fs, z_message_t *msg) {
    raleighfs_object_t object;
    raleighfs_errno_t errno;

    /* lock the semantic layer */
    if (!__begin_write_process(&(fs->lock))) {
        z_message_yield(msg);
        return;
    }

    /* try to open the file */
    errno = raleighfs_semantic_open(fs, &object, z_message_object(msg));

    /* unlock the semantic layer */
    __end_process(&(fs->lock));

    /* TODO: CHECK ERRNO */

    /* What's next operation? */
    switch (RALEIGHFS_MSG_TYPE(z_message_type(msg))) {
        case RALEIGHFS_UNLINK:
            z_message_set_sub_task(msg, __task_exec_unlink, object.internal);
            z_message_yield(msg);
            break;
        default:
            z_message_set_sub_task(msg, __task_exec_object, object.internal);
            z_message_yield(msg);
            break;
    }    
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

static void __raleighfs_exec_semantic (raleighfs_t *fs, z_message_t *msg) {
    raleighfs_errno_t errno;

    /* lock the semantic layer */
    if (!__begin_write_process(&(fs->lock))) {
        z_message_yield(msg);
        return;
    }

    switch (RALEIGHFS_MSG_TYPE(z_message_type(msg))) {
        case RALEIGHFS_CREATE:
            Z_LOG("raleighfs create...\n");
            errno = __exec_semantic_create(fs, z_message_object(msg), msg);
            break;
        case RALEIGHFS_RENAME:
            Z_LOG("raleighfs rename...\n");
            errno = __exec_semantic_rename(fs, z_message_object(msg), msg);
            break;
        case RALEIGHFS_EXISTS:
            Z_LOG("raleighfs exists...\n");
            errno = raleighfs_semantic_exists(fs, z_message_object(msg));
            break;
        default:
            Z_LOG("raleighfs semantic unknown operation...\n");
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    /* unlock the semantic layer */
    __end_process(&(fs->lock));

    /* Set state for message */
    z_message_set_state(msg, errno);
}

/* ============================================================================
 *  MessageQ Executor
 */
void raleighfs_execute (raleighfs_t *fs, z_message_t *message) {
    if (RALEIGHFS_OBJECT_MSG(z_message_type(message)))
        __raleighfs_exec_open(fs, message);
    else
        __raleighfs_exec_semantic(fs, message);
}

