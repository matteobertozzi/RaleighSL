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

#include "private.h"

#define raleighfs_type_id       54

raleighfs_t *raleighfs_alloc (raleighfs_t *fs, z_memory_t *memory) {
    if ((fs = z_object_alloc(memory, fs, raleighfs_t)) == NULL)
        return(NULL);

    if (__plugin_table_alloc(fs)) {
        z_object_free(fs);
        return(NULL);
    }

    return(fs);
}

void raleighfs_free (raleighfs_t *fs) {
    __plugin_table_free(fs);
    z_object_free(fs);
}
#include <stdio.h>
raleighfs_errno_t raleighfs_create (raleighfs_t *fs,
                                    raleighfs_device_t *device,
                                    raleighfs_key_plug_t *key,
                                    raleighfs_format_plug_t *format,
                                    raleighfs_space_plug_t *space,
                                    raleighfs_semantic_plug_t *semantic)
{
    raleighfs_errno_t errno;

    /* Initialize file-system struct */
    fs->device = device;
    fs->key.plug = key;
    fs->space.plug = space;
    fs->format.plug = format;
    fs->semantic.plug = semantic;
    fs->objcache.plug = &raleighfs_objcache_tree;

    /* Open object cache */
    if ((errno = __objcache_call(fs, open)))
        return(errno);

    /* Create new format */
    if ((errno = __format_call_unrequired(fs, init))) {
        (void)__objcache_call(fs, close);
        return(errno);
    }

    /* Create key */
    if ((errno = __key_call_unrequired(fs, init))) {
        (void)__format_call_unrequired(fs, unload);
        (void)__objcache_call(fs, close);
        return(errno);
    }

    /* Create new space */
    if ((errno = __space_call_unrequired(fs, init))) {
        (void)__key_call_unrequired(fs, unload);
        (void)__format_call_unrequired(fs, unload);
        (void)__objcache_call(fs, close);
        return(errno);
    }

    /* Create new semantic layer */
    if ((errno = __semantic_call_unrequired(fs, init))) {
        (void)__space_call_unrequired(fs, unload);
        (void)__key_call_unrequired(fs, unload);
        (void)__format_call_unrequired(fs, unload);
        (void)__objcache_call(fs, close);
        return(errno);
    }

    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_open (raleighfs_t *fs,
                                  raleighfs_device_t *device)
{
    raleighfs_errno_t errno;

    /* Initialize file-system struct */
    fs->device = device;

    /* Open object cache */
    if ((errno = __objcache_call(fs, open)))
        return(errno);

    /* Load File-system format */
    if ((errno = __format_call_unrequired(fs, load))) {
        (void)__objcache_call(fs, close);
        return(errno);
    }

    /* Load key */
    if ((errno = __key_call_unrequired(fs, load))) {
        (void)__format_call_unrequired(fs, unload);
        (void)__objcache_call(fs, close);
        return(errno);
    }

    /* Load space allocator */
    if ((errno = __space_call_unrequired(fs, load))) {
        (void)__key_call_unrequired(fs, unload);
        (void)__format_call_unrequired(fs, unload);
        (void)__objcache_call(fs, close);
        return(errno);
    }

    /* Load semantic layer */
    if ((errno = __semantic_call_unrequired(fs, load))) {
        (void)__semantic_call_unrequired(fs, unload);
        (void)__key_call_unrequired(fs, unload);
        (void)__format_call_unrequired(fs, unload);
        (void)__objcache_call(fs, close);
        return(errno);
    }

    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_close (raleighfs_t *fs) {
    raleighfs_errno_t errno;

    if ((errno = raleighfs_sync(fs)))
        return(errno);

    (void)__space_call_unrequired(fs, unload);
    (void)__key_call_unrequired(fs, unload);
    (void)__format_call_unrequired(fs, unload);
    (void)__objcache_call(fs, close);
    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_sync (raleighfs_t *fs) {
    raleighfs_errno_t errno;

    if ((errno = __objcache_call(fs, sync)))
        return(errno);

    if ((errno = __space_call_unrequired(fs, sync)))
        return(errno);

    if ((errno = __format_call_unrequired(fs, sync)))
        return(errno);

    if ((errno = __device_call_unrequired(fs, sync)))
        return(errno);

    return(RALEIGHFS_ERRNO_NONE);
}

