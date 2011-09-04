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

#include <raleighfs/semantic.h>
#include <raleighfs/key.h>

#include <zcl/debug.h>

#include "private.h"

raleighfs_errno_t raleighfs_semantic_touch (raleighfs_t *fs,
                                            raleighfs_object_plug_t *plug,
                                            const z_rdata_t *name)
{
    raleighfs_object_t object;
    raleighfs_errno_t errno;

    if ((errno = raleighfs_semantic_create(fs, &object, plug, name)))
        return(errno);

    raleighfs_semantic_close(fs, &object);
    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_semantic_create (raleighfs_t *fs,
                                             raleighfs_object_t *object,
                                             raleighfs_object_plug_t *plug,
                                             const z_rdata_t *name)
{
    raleighfs_errno_t errno;
    raleighfs_key_t key;

    /* Resolve: name to key */
    if ((errno = raleighfs_key_object(fs, &key, name)))
        return(errno);

    /* Check if object exists and it is already in memory */
    if (!(errno = __objcache_lookup(fs, object, &key)))
        return(RALEIGHFS_ERRNO_OBJECT_EXISTS);

    /* Allocate object */
    if ((errno = __object_alloc(fs, object, &key)))
        return(errno);

    /* Assign plug to object */
    object->internal->plug = plug;

    /* Semantic create metadata */
    if ((errno = __semantic_call_required(fs, create, object, name)))
        return(errno);

    /* Object create */
    if ((errno = __object_call_required(fs, object, create))) {
        (void)__semantic_call_unrequired(fs, unlink, object);
        (void)__semantic_call_required(fs, close, object);
        return(errno);
    }

    /* Add object to in-memory cache */
    if ((errno = __objcache_insert(fs, object))) {
        (void)__object_call_unrequired(fs, object, close);
        (void)__semantic_call_unrequired(fs, unlink, object);
        (void)__semantic_call_unrequired(fs, close, object);
        return(errno);
    }

    __observer_notify_create(fs, name);
    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_semantic_open (raleighfs_t *fs,
                                           raleighfs_object_t *object,
                                           const z_rdata_t *name)
{
    raleighfs_errno_t errno;
    raleighfs_key_t key;

    /* Resolve: name to key */
    if ((errno = raleighfs_key_object(fs, &key, name)))
        return(errno);

    /* Check if object is already in memory */
    if (!(errno = __objcache_lookup(fs, object, &key))) {
        __object_inc_ref(object);
        return(RALEIGHFS_ERRNO_NONE);
    }

    /* Allocate object */
    if ((errno = __object_alloc(fs, object, &key)))
        return(errno);

    /* Semantic open, lookup metadata */
    if ((errno = __semantic_call_required(fs, open, object))) {
        __object_free(fs, object);
        return(errno);
    }

    if ((errno = __object_call_required(fs, object, open))) {
        (void)__semantic_call_unrequired(fs, close, object);
        __object_free(fs, object);
        return(errno);
    }

    /* Add object to in-memory cache */
    if ((errno = __objcache_insert(fs, object))) {
        (void)__object_call_unrequired(fs, object, close);
        (void)__semantic_call_unrequired(fs, close, object);
        __object_free(fs, object);
        return(errno);
    }

    __observer_notify_open(fs, name);
    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_semantic_close (raleighfs_t *fs,
                                            raleighfs_object_t *object)
{
    raleighfs_errno_t errno;

    /* If there're more references, delay close */
    if (__object_dec_ref(object) > 0)
        return(RALEIGHFS_ERRNO_NONE);

    /* Sync everything before close */
    if ((errno = raleighfs_semantic_sync(fs, object)))
        return(errno);

    /* Call object close */
    if ((errno = __object_call_unrequired(fs, object, close)))
        return(errno);

    /* Close semantic layer object */
    if ((errno = __semantic_call_required(fs, close, object)))
        return(errno);

    __objcache_remove(fs, object);
    __object_free(fs, object);

    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_semantic_sync (raleighfs_t *fs,
                                           raleighfs_object_t *object)
{
    raleighfs_errno_t errno;

    if ((errno = __object_call_required(fs, object, sync)))
        return(errno);

     return(__semantic_call_required(fs, sync, object));
}

raleighfs_errno_t raleighfs_semantic_unlink (raleighfs_t *fs,
                                             raleighfs_object_t *object)
{
    raleighfs_errno_t errno;

    if ((errno = __object_call_required(fs, object, unlink)))
        return(errno);

    if ((errno = __semantic_call_required(fs, unlink, object)))
        return(errno);

    /* __observer_notify_unlink(fs, object->internal->name); */

    __objcache_remove(fs, object);
    __object_free(fs, object);

    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_semantic_exists (raleighfs_t *fs,
                                             const z_rdata_t *name)
{
    raleighfs_object_t object;
    raleighfs_errno_t errno;
    raleighfs_key_t key;

    /* Resolve: name to key */
    if ((errno = raleighfs_key_object(fs, &key, name)))
        return(errno);

    /* Check if object is already in memory */
    if (!(errno = __objcache_lookup(fs, &object, &key)))
        return(RALEIGHFS_ERRNO_INFO | RALEIGHFS_ERRNO_OBJECT_EXISTS);

    /* Allocate object */
    if ((errno = __object_alloc(fs, &object, &key)))
        return(errno);

    /* Semantic open, lookup metadata */
    switch ((errno = __semantic_call_required(fs, open, &object))) {
        case RALEIGHFS_ERRNO_OBJECT_EXISTS:
        case RALEIGHFS_ERRNO_OBJECT_NOT_FOUND:
            errno |= RALEIGHFS_ERRNO_INFO;
            break;
        default:
            break;
    }

    /* Free Object */
    __object_free(fs, &object);

    return(errno);
}

raleighfs_errno_t raleighfs_semantic_rename (raleighfs_t *fs,
                                             const z_rdata_t *old_name,
                                             const z_rdata_t *new_name)
{
    raleighfs_object_t fake_object;
    raleighfs_object_t object;
    raleighfs_errno_t errno;
    raleighfs_key_t new_key;
    raleighfs_key_t old_key;
    int in_memory, renamed;

    /* Resolve: name to key */
    if ((errno = raleighfs_key_object(fs, &old_key, old_name)))
        return(errno);

    if ((errno = raleighfs_key_object(fs, &new_key, new_name)))
        return(errno);

    /* If object is already in memory,
     * create new key object and add to objcache.
     */
    if ((in_memory = !__objcache_lookup(fs, &object, &old_key))) {
        if ((errno = __object_copy(fs, &fake_object, &new_key, &object)))
            return(errno);

        if ((errno = __objcache_insert(fs, &fake_object))) {
            __object_free(fs, &fake_object);
            return(errno);
        }
    }

    /* Do semantic rename! (disk changes) */
    renamed = 0;
    if (!(errno = __semantic_call_required(fs, rename,
                                           &old_key, old_name,
                                           &new_key, new_name)))
    {
        renamed = 1;
        __observer_notify_rename(fs, old_name, new_name);
    }

    /* If object is already in memory, remove the old key from objcache */
    if (in_memory) {
        if (renamed) {
            __objcache_remove(fs, &object);

            /* Do a replace fake_object/object */
            __object_set_key(fs, &object, &new_key);
            if (__objcache_insert(fs, &object))
                Z_BUG("__objcache_insert() cant fail on replace!\n");
        } else {
            __objcache_remove(fs, &fake_object);
        }

        __object_free(fs, &fake_object);
    }

    return(errno);
}

