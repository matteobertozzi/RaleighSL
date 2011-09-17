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

#ifndef _Z_EXTENTS_TREE_H_
#define _Z_EXTENTS_TREE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/tree.h>

#define Z_EXTENT_TREE(x)            Z_CAST(z_extent_tree_t, x)
#define Z_EXTENT(x)                 Z_CAST(z_extent_t, x)

#define Z_EXTENT_TYPE               z_extent_t __base__;

Z_TYPEDEF_CONST_STRUCT(z_extent_plug)
Z_TYPEDEF_STRUCT(z_extent_tree)
Z_TYPEDEF_STRUCT(z_extent)

struct z_extent_plug {
    int     (*head_trim)      (void *user_data,
                               z_extent_t *extent,
                               uint64_t trim);
    int     (*tail_trim)      (void *user_data,
                               z_extent_t *extent,
                               uint64_t trim);
    int     (*split)          (void *user_data,
                               const z_extent_t *extent,
                               uint64_t offset,
                               z_extent_t **left,
                               z_extent_t **right);
};

struct z_extent_tree {
    Z_TREE_TYPE

    z_extent_plug_t *plug;
};

struct z_extent {
    uint64_t offset;
    uint64_t length;
};

z_extent_tree_t *  z_extent_tree_alloc      (z_extent_tree_t *tree,
                                             z_memory_t *memory,
                                             z_extent_plug_t *plug,
                                             z_mem_free_t free_data,
                                             void *user_data);

void               z_extent_tree_free       (z_extent_tree_t *tree);

int                z_extent_tree_insert     (z_extent_tree_t *tree,
                                             z_extent_t *extent);
int                z_extent_tree_remove     (z_extent_tree_t *tree,
                                             uint64_t start,
                                             uint64_t length);

__Z_END_DECLS__

#endif /* !_Z_EXTENTS_TREE_H_ */

