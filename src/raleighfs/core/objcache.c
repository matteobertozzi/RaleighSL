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
#include <raleighfs/types.h>
#include <raleighfs/key.h>

#include <zcl/tree.h>

#define __CACHE_TREE(fs)                    Z_TREE((fs)->objcache.data.ptr)
#define __CACHE_TREE_SET(fs, tree)          (fs)->objcache.data.ptr = (tree)

static int __cache_object_compare (void *fs, const void *a, const void *b) {
    return(raleighfs_key_compare(RALEIGHFS(fs),
                                 RALEIGHFS_OBJDATA_KEY(a),
                                 RALEIGHFS_OBJDATA_KEY(b)));
}

static int __cache_objkey_compare (void *fs, const void *a, const void *b) {
    return(raleighfs_key_compare(RALEIGHFS(fs),
                                 RALEIGHFS_OBJDATA_KEY(a),
                                 RALEIGHFS_KEY(b)));
}

static raleighfs_errno_t __cache_open (raleighfs_t *fs) {
    z_tree_t *tree;

    if ((tree = z_tree_alloc(NULL, z_object_memory(fs), &z_tree_red_black,
                             __cache_object_compare, NULL, fs)) == NULL)
    {
        return(RALEIGHFS_ERRNO_NO_MEMORY);
    }

    __CACHE_TREE_SET(fs, tree);
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __cache_close (raleighfs_t *fs) {
    z_tree_free(__CACHE_TREE(fs));
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __cache_sync (raleighfs_t *fs) {
    raleighfs_objdata_t *objdata;
    raleighfs_object_t object;
    raleighfs_errno_t errno;
    z_tree_iter_t iter;

    z_tree_iter_init(&iter, __CACHE_TREE(fs));

    objdata = RALEIGHFS_OBJDATA(z_tree_iter_lookup_min(&iter));
    while (objdata != NULL) {
        object.internal = objdata;

        if ((errno = raleighfs_semantic_sync(fs, &object)))
            return(errno);

        objdata = RALEIGHFS_OBJDATA(z_tree_iter_next(&iter));
    }

    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __cache_insert (raleighfs_t *fs,
                                         raleighfs_object_t *object)
{
    if (z_tree_insert(__CACHE_TREE(fs), object->internal))
        return(RALEIGHFS_ERRNO_NO_MEMORY);
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __cache_remove (raleighfs_t *fs,
                                         raleighfs_object_t *object)
{
    z_tree_remove(__CACHE_TREE(fs), object->internal);
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __cache_lookup (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         const raleighfs_key_t *key)
{
    z_tree_t *tree = __CACHE_TREE(fs);
    void *obj;

    if ((obj = z_tree_lookup_custom(tree, __cache_objkey_compare, key)) == NULL)
        return(RALEIGHFS_ERRNO_OBJECT_NOT_FOUND);

    object->internal = RALEIGHFS_OBJDATA(obj);
    return(RALEIGHFS_ERRNO_NONE);
}

const uint8_t RALEIGHFS_OBJCACHE_TREE_UUID[16] = {
    5,131,255,215,134,200,78,185,190,50,255,215,224,118,16,133,
};

raleighfs_objcache_plug_t raleighfs_objcache_tree = {
    .info = {
        .type = RALEIGHFS_PLUG_TYPE_OBJCACHE,
        .uuid = RALEIGHFS_OBJCACHE_TREE_UUID,
        .description = "Red-Black Tree Objects Cache",
        .label = "objcache-tree",
    },

    .open   = __cache_open,
    .close  = __cache_close,
    .sync   = __cache_sync,

    .insert = __cache_insert,
    .remove = __cache_remove,
    .lookup = __cache_lookup,
};

