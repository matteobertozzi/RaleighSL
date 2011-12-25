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

#include <raleighfs/object.h>

#include <zcl/thread.h>
#include <zcl/memcpy.h>

#include "private.h"

raleighfs_errno_t __object_alloc (raleighfs_t *fs,
                                  raleighfs_object_t *object,
                                  const raleighfs_key_t *key)
{
    raleighfs_objdata_t *objdata;

    if ((objdata = z_object_struct_alloc(fs, raleighfs_objdata_t)) == NULL)
        return(RALEIGHFS_ERRNO_NO_MEMORY);

    /* Initialize Object's attributes */
    z_memcpy(&(objdata->key), key, sizeof(raleighfs_key_t));
    objdata->devbufs = NULL;
    objdata->membufs = NULL;
    objdata->plug = NULL;
    objdata->refs = 1U;

    /* Initialize Object Read-Write ock */
    __rwlock_init(&(objdata->lock));

    object->internal = objdata;
    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t __object_copy (raleighfs_t *fs,
                                 raleighfs_object_t *objdst,
                                 const raleighfs_key_t *new_key,
                                 const raleighfs_object_t *objsrc)
{
    raleighfs_objdata_t *objdata;

    if ((objdata = z_object_struct_alloc(fs, raleighfs_objdata_t)) == NULL)
        return(RALEIGHFS_ERRNO_NO_MEMORY);

    z_memcpy(&(objdata->key), new_key, sizeof(raleighfs_key_t));
    objdata->plug = objsrc->internal->plug;
    objdata->devbufs = objsrc->internal->devbufs;
    objdata->membufs = objsrc->internal->membufs;
    objdata->refs = objsrc->internal->refs;

    objdst->internal = objdata;
    return(RALEIGHFS_ERRNO_NONE);
}

void __object_free (raleighfs_t *fs, raleighfs_object_t *object) {
    raleighfs_objdata_t *objdata = object->internal;
    __rwlock_uninit(&(objdata->lock));
    z_object_struct_free(fs, raleighfs_objdata_t, objdata);
}

void __object_set_key (raleighfs_t *fs,
                       raleighfs_object_t *object,
                       raleighfs_key_t *key)
{
    z_memcpy(&(object->internal->key), key, sizeof(raleighfs_key_t));
}

/* ============================================================================
 *  Object public 'query' methods
 *
 *  TODO: Add observer for pre-do
 */
raleighfs_errno_t raleighfs_object_query (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *message)
{
    return(__object_call_required(fs, object, query, message));
}

raleighfs_errno_t raleighfs_object_insert (raleighfs_t *fs,
                                           raleighfs_object_t *object,
                                           z_message_t *message)
{
    raleighfs_errno_t errno;

    if (!(errno = __object_call_required(fs, object, insert, message)))
        __observer_notify_insert(fs, object, message);

    return(errno);
}

raleighfs_errno_t raleighfs_object_update (raleighfs_t *fs,
                                           raleighfs_object_t *object,
                                           z_message_t *message)
{
    raleighfs_errno_t errno;

    if (!(errno = __object_call_required(fs, object, update, message)))
        __observer_notify_update(fs, object, message);

    return(errno);
}

raleighfs_errno_t raleighfs_object_remove (raleighfs_t *fs,
                                           raleighfs_object_t *object,
                                           z_message_t *message)
{
    raleighfs_errno_t errno;

    if (!(errno = __object_call_required(fs, object, remove, message)))
        __observer_notify_remove(fs, object, message);

    return(errno);
}

raleighfs_errno_t raleighfs_object_ioctl (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *message)
{
    raleighfs_errno_t errno;

    if (!(errno = __object_call_required(fs, object, ioctl, message)))
        __observer_notify_ioctl(fs, object, message);

    return(errno);
}

