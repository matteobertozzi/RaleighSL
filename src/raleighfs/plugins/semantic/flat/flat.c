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

#include <raleighfs/types.h>

#include "flat.h"

static raleighfs_errno_t __semantic_init (raleighfs_t *fs) {
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __semantic_load (raleighfs_t *fs) {
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __semantic_unload (raleighfs_t *fs) {
    return(RALEIGHFS_ERRNO_NONE);
}


static raleighfs_errno_t __semantic_create (raleighfs_t *fs,
                                            raleighfs_object_t *object,
                                            const z_slice_t *name)
{
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __semantic_open (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
    return(RALEIGHFS_ERRNO_OBJECT_NOT_FOUND);
}

static raleighfs_errno_t __semantic_close (raleighfs_t *fs,
                                           raleighfs_object_t *object)
{
    return(RALEIGHFS_ERRNO_NOT_IMPLEMENTED);
}


static raleighfs_errno_t __semantic_sync (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
    return(RALEIGHFS_ERRNO_NOT_IMPLEMENTED);
}

static raleighfs_errno_t __semantic_unlink (raleighfs_t *fs,
                                            raleighfs_object_t *object)
{
    return(RALEIGHFS_ERRNO_NONE);
}


static raleighfs_errno_t __semantic_rename (raleighfs_t *fs,
                                            const raleighfs_key_t *old_key,
                                            const z_slice_t *old_name,
                                            const raleighfs_key_t *new_key,
                                            const z_slice_t *new_name)
{
    return(RALEIGHFS_ERRNO_NONE);
}

const uint8_t RALEIGHFS_SEMANTIC_FLAT_UUID[16] = {
    13,146,163,151,99,185,71,244,183,123,78,216,119,213,35,14,
};

raleighfs_semantic_plug_t raleighfs_semantic_flat = {
    .info = {
        .type = RALEIGHFS_PLUG_TYPE_SEMANTIC,
        .uuid = RALEIGHFS_SEMANTIC_FLAT_UUID,
        .description = "Flat namespace",
        .label = "semantic-flat",
    },

    .init       = __semantic_init,
    .load       = __semantic_load,
    .unload     = __semantic_unload,

    .create     = __semantic_create,
    .open       = __semantic_open,
    .close      = __semantic_close,
    .sync       = __semantic_sync,
    .unlink     = __semantic_unlink,

    .rename     = __semantic_rename,
};

