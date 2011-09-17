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

#include <zcl/extents.h>

#define __extent_lookup_left(tree, extent)                                  \
    Z_EXTENT(z_tree_lookup_custom(Z_TREE(tree), __lookup_left_compare, extent))

#define __extent_lookup_right(tree, extent)                                 \
    Z_EXTENT(z_tree_lookup_custom(Z_TREE(tree), __lookup_right_compare, extent))

#define __tree_data(tree)         (Z_TREE(tree)->info.user_data)

static int __extent_compare (void *user_data,
                             const void *a,
                             const void *b)
{
    const z_extent_t *ea = Z_EXTENT(a);
    const z_extent_t *eb = Z_EXTENT(b);

    uint64_t ah = ea->offset + ea->length - 1;
    uint64_t bh = eb->offset + eb->length - 1;
    return((ah < bh) ? -1 : ((ah > bh) ? 1 : 0));
}

static int __lookup_left_compare (void *user_data,
                                  const void *extent,
                                  const void *ref)
{
    const z_extent_t *ea = (const z_extent_t *)ref;
    const z_extent_t *eb = (const z_extent_t *)extent;
    uint64_t ah = ea->offset + ea->length - 1;
    uint64_t bh = eb->offset + eb->length - 1;

    /* check overlap */
    if (ea->offset <= bh && eb->offset <= ah) {
        if (ea->offset >= eb->offset && ea->offset <= bh)
            return(0);
        return(1);
    }

    return((ah < bh) ? 1 : ((ah > bh) ? -1 : 0));
}

static int __lookup_right_compare (void *user_data,
                                   const void *extent,
                                   const void *ref)
{
    const z_extent_t *ea = (const z_extent_t *)ref;
    const z_extent_t *eb = (const z_extent_t *)extent;
    uint64_t ah = ea->offset + ea->length - 1;
    uint64_t bh = eb->offset + eb->length - 1;

    /* check overlap */
    if (ea->offset <= bh && eb->offset <= ah) {
        if (ah >= eb->offset && ah <= bh)
            return(0);
        return(-1);
    }

    return((ah < bh) ? 1 : ((ah > bh) ? -1 : 0));
}

static void __extents_right_shift (z_extent_tree_t *tree,
                                   z_extent_t *extent)
{
    z_tree_iter_t iter;
    z_extent_t *e;

    z_tree_iter_init(&iter, Z_TREE(tree));
    e = Z_EXTENT(z_tree_iter_lookup_min(&iter));
    while (e != NULL) {
        if (e->offset >= extent->offset)
            e->offset += extent->length;
        e = Z_EXTENT(z_tree_iter_next(&iter));
    }
}


static void __extents_left_shift (z_extent_tree_t *tree,
                                  z_extent_t *extent)
{
    z_tree_iter_t iter;
    z_extent_t *e;

    z_tree_iter_init(&iter, Z_TREE(tree));
    e = Z_EXTENT(z_tree_iter_lookup_min(&iter));
    while (e != NULL) {
        if (e->offset > extent->offset)
            e->offset -= extent->length;
        e = Z_EXTENT(z_tree_iter_next(&iter));
    }
}

static int __extent_trim (z_extent_tree_t *tree,
                          z_extent_t *extent,
                          uint64_t trim)
{
    if (tree->plug->tail_trim(__tree_data(tree), extent, trim))
        return(1);

    extent->length -= trim;
    return(0);
}

static int __extent_pre_trim (z_extent_tree_t *tree,
                              z_extent_t *extent,
                              uint64_t trim)
{
    if (tree->plug->head_trim(__tree_data(tree), extent, trim))
        return(1);

    extent->length -= trim;
    return(0);
}

static int __extent_split (z_extent_tree_t *tree,
                           z_extent_t *extent,
                           uint64_t offset)
{
    z_extent_t *right;
    z_extent_t *left;

    if (tree->plug->split(__tree_data(tree), extent, offset, &left, &right))
        return(1);

    if (extent != left && extent != right) {
        z_tree_remove(Z_TREE(tree), extent);
        z_tree_insert(Z_TREE(tree), left);
        z_tree_insert(Z_TREE(tree), right);
    } else if (left != extent) {
        z_tree_insert(Z_TREE(tree), left);
    } else /* if (right != extent) */ {
        z_tree_insert(Z_TREE(tree), right);
    }

    return(0);
}

z_extent_tree_t *z_extent_tree_alloc (z_extent_tree_t *tree,
                                      z_memory_t *memory,
                                      z_extent_plug_t *plug,
                                      z_mem_free_t free_data,
                                      void *user_data)
{
    if ((tree = z_object_alloc(memory, tree, z_extent_tree_t)) == NULL)
        return(NULL);

    if (z_tree_init(Z_TREE(tree), &z_tree_avl,
                    __extent_compare, free_data, user_data))
    {
        z_tree_free(Z_TREE(tree));
        return(NULL);
    }

    tree->plug = plug;

    return(tree);
}

void z_extent_tree_free (z_extent_tree_t *tree) {
    z_tree_free(Z_TREE(tree));
}

int z_extent_tree_insert (z_extent_tree_t *tree,
                          z_extent_t *extent)
{
    z_extent_t *left;

    if ((left = __extent_lookup_left(tree, extent)) != NULL) {
        if (left->offset < extent->offset) {
            /* Split extent */
            if (__extent_split(tree, left, extent->offset))
                return(1);
        }

        /* Shift everything to the right */
        __extents_right_shift(tree, extent);
    }

    /* TODO: Optimize compaction... */
    if (z_tree_insert(Z_TREE(tree), extent))
        return(2);

    /* tree->length += extent->length; */
    return(0);
}

int z_extent_tree_remove (z_extent_tree_t *tree,
                          uint64_t offset,
                          uint64_t length)
{
    z_extent_t extent;
    z_extent_t *right;
    z_extent_t *left;
    uint64_t rh;
    uint64_t h;

    extent.offset = offset;
    extent.length = length;
    h = offset + length - 1;

    if ((left = __extent_lookup_left(tree, &extent)) == NULL)
        return(1);

    if ((right = __extent_lookup_right(tree, &extent)) == NULL)
        right = left;

    rh = right->offset + right->length - 1;
    if (left == right) {
        if (offset == left->offset && h == rh) {
            z_tree_remove(Z_TREE(tree), left);
        } else if (offset == left->offset) {
            __extent_pre_trim(tree, left, length);
        } else if (h == rh) {
            __extent_trim(tree, left, length);
        } else {
            __extent_split(tree, left, h);
            __extent_trim(tree, left, length);
        }
    } else {
        z_tree_remove_range(Z_TREE(tree), left, right);

        if (offset == left->offset && h == rh) {
            z_tree_remove(Z_TREE(tree), left);
            z_tree_remove(Z_TREE(tree), right);
        } else if (offset == left->offset) {
            z_tree_remove(Z_TREE(tree), left);
            right->offset -= length - (right->length - (rh - h));
            __extent_pre_trim(tree, right, right->length - (rh - h));
        } else if (h == rh) {
            z_tree_remove(Z_TREE(tree), right);
            __extent_trim(tree, left, (left->offset + left->length) - offset);
        } else {
            right->offset -= length - (right->length - (rh - h));
            __extent_trim(tree, left, (left->offset + left->length) - offset);
            __extent_pre_trim(tree, right, right->length - (rh - h));
        }
    }

    __extents_left_shift(tree, &extent);
    /* tree->length -= extent.length; */

    return(0);
}

