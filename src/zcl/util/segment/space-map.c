/*
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

#include <zcl/space-map.h>
#include <zcl/global.h>

struct segment_node {
  z_tree_node_t offset_node;
  z_tree_node_t size_node;
  /* start = offset_node->uflags32 */
  /* size   = size_node->uflags32 */
};

/* ============================================================================
 *  PRIVATE Space Map Trees Macros
 */
#define __segment_offset_tree_attach(space_map, segment)                      \
  z_tree_node_attach(&__segment_offset_tree, &((space_map)->offset_root),     \
                     &((segment)->offset_node), space_map)

#define __segment_offset_tree_detach(space_map, segment)                      \
  z_tree_node_detach(&__segment_offset_tree, &((space_map)->offset_root),     \
                     &((segment)->offset_node), space_map)

#define __segment_size_tree_attach(space_map, segment)                        \
  z_tree_node_attach(&__segment_size_tree, &((space_map)->size_root),         \
                     &((segment)->size_node), space_map)

#define __segment_size_tree_detach(space_map, segment)                        \
  z_tree_node_detach(&__segment_size_tree, &((space_map)->size_root),         \
                     &((segment)->size_node), space_map)

/* ============================================================================
 *  PRIVATE Segment Macros
 */
#define __segment_start(segment)     ((segment)->offset_node.uflags32)
#define __segment_size(segment)      ((segment)->size_node.uflags32)
#define __segment_end(segment)       (__segment_start(segment) + __segment_size(segment))

/* ============================================================================
 *  PRIVATE Segment Map Methods
 */
static struct segment_node *__segment_alloc (uint32_t start, uint32_t size) {
  struct segment_node *segment;

  segment = z_memory_struct_alloc(z_global_memory(), struct segment_node);
  if (Z_MALLOC_IS_NULL(segment))
    return(NULL);

  segment->offset_node.uflags32 = start;
  segment->size_node.uflags32 = size;
  return(segment);
}

static void __segment_free (struct segment_node *segment) {
  z_memory_struct_free(z_global_memory(), struct segment_node, segment);
}

/* ============================================================================
 *  PUBLIC Segment Map Offset Tree Methods
 */
static int __segment_closest_start (void *udata,
                                     const void *vnode,
                                     const void *vkey,
                                     uint64_t *delta)
{
  struct segment_node *node = z_container_of(vnode, struct segment_node, offset_node);
  uint32_t start = Z_UINT32_PTR_VALUE(vkey);

  if (__segment_start(node) < start) {
    *delta = (start - __segment_start(node));
    return(-1);
  }

  if (__segment_start(node) > start) {
    *delta = (__segment_start(node) - start);
    return(1);
  }

  *delta = 0;
  return(0);
}

static int __segment_start_cmp (void *udata, const void *a, const void *b) {
  struct segment_node *a_node = z_container_of(a, struct segment_node, offset_node);
  struct segment_node *b_node = z_container_of(b, struct segment_node, offset_node);
  return(z_cmp(__segment_start(a_node), __segment_start(b_node)));
}

static int __segment_start_key_cmp (void *udata, const void *vnode, const void *vkey) {
  struct segment_node *node = z_container_of(vnode, struct segment_node, offset_node);
  uint32_t start = Z_UINT32_PTR_VALUE(vkey);
  return(z_cmp(__segment_start(node), start));
}

static int __segment_end_key_cmp (void *udata, const void *vnode, const void *vkey) {
  struct segment_node *node = z_container_of(vnode, struct segment_node, offset_node);
  uint32_t end = Z_UINT32_PTR_VALUE(vkey);
  return(z_cmp(__segment_end(node), end));
}

static void __segment_node_free (void *udata, void *vnode) {
  struct segment_node *node = z_container_of(vnode, struct segment_node, offset_node);
  __segment_free(node);
}

static const z_tree_info_t __segment_offset_tree = {
  .plug         = &z_tree_avl,
  .node_compare = __segment_start_cmp,
  .key_compare  = __segment_start_cmp,
  .node_free    = NULL,
};

/* ============================================================================
 *  PUBLIC Segment Map Offset Tree Methods
 */
static int __segment_closest_size (void *udata,
                                   const void *vnode,
                                   const void *vkey,
                                   uint64_t *delta)
{
  struct segment_node *node = z_container_of(vnode, struct segment_node, size_node);
  uint32_t size = Z_UINT32_PTR_VALUE(vkey);

  if (__segment_size(node) < size) {
    *delta = (size - __segment_size(node));
    return(-1);
  }

  if (__segment_size(node) > size) {
    *delta = (__segment_size(node) - size);
    return(1);
  }

  *delta = 0;
  return(0);
}

static int __segment_size_cmp (void *udata, const void *a, const void *b) {
  struct segment_node *a_node = z_container_of(a, struct segment_node, size_node);
  struct segment_node *b_node = z_container_of(b, struct segment_node, size_node);
  int cmp;
  if ((cmp = z_cmp(__segment_size(a_node), __segment_size(b_node))))
    return(cmp);
  return(z_cmp(__segment_start(a_node), __segment_start(b_node)));
}

static const z_tree_info_t __segment_size_tree = {
  .plug         = &z_tree_avl,
  .node_compare = __segment_size_cmp,
  .key_compare  = __segment_size_cmp,
  .node_free    = NULL,
};

/* ============================================================================
 *  PUBLIC Segment Map Methods
 */
void z_space_map_open (z_space_map_t *self) {
  self->offset_root = NULL;
  self->size_root = NULL;
  self->available = 0;
}

void z_space_map_close (z_space_map_t *self) {
  z_tree_node_clear(self->offset_root, __segment_node_free, self);
  self->offset_root = NULL;
  self->size_root = NULL;
  self->available = 0;
}

int z_space_map_add (z_space_map_t *self, uint32_t start, uint32_t size) {
  struct segment_node *closest_segment;
  z_tree_node_t *node;

  node = z_tree_node_closest(self->offset_root, __segment_closest_start, &start, self);
  if (Z_UNLIKELY(node == NULL)) {
    struct segment_node *segment;

    /* Add the new request nothing close to it */
    segment = __segment_alloc(start, size);
    if (Z_MALLOC_IS_NULL(segment))
      return(1);

    __segment_offset_tree_attach(self, segment);
    __segment_size_tree_attach(self, segment);
    self->available += size;
    return(0);
  }

  closest_segment = z_container_of(node, struct segment_node, offset_node);
  if (__segment_start(closest_segment) == (start + size)) {
    /*
     *        new segment
     *  A |-----------------| B
     *                        closest segment
     *                    B |-----------------| C
     */
    __segment_offset_tree_detach(self, closest_segment);
    __segment_size_tree_detach(self, closest_segment);

    __segment_start(closest_segment) = start;
    __segment_size(closest_segment) += size;

    /* Try to merge with a previous segment if any */
    node = z_tree_node_lookup(self->offset_root, __segment_end_key_cmp, &start, self);
    if (node != NULL) {
      struct segment_node *pre_segment;

      pre_segment = z_container_of(node, struct segment_node, offset_node);
      __segment_size_tree_detach(self, pre_segment);
      __segment_size(pre_segment) += __segment_size(closest_segment);
      __segment_size_tree_attach(self, pre_segment);

      __segment_free(closest_segment);
    } else {
      __segment_offset_tree_attach(self, closest_segment);
      __segment_size_tree_attach(self, closest_segment);
    }
  } else if (start == __segment_end(closest_segment)) {
    uint32_t next_start;
    /*
     *      closest segment
     *  A |-----------------| B
     *                          new segment
     *                    B |-----------------| C
     */
    __segment_size_tree_detach(self, closest_segment);
    __segment_size(closest_segment) += size;

    /* Try to merge with a next segment if any */
    next_start = __segment_end(closest_segment);
    node = z_tree_node_lookup(self->offset_root, __segment_start_key_cmp, &next_start, self);
    if (node != NULL) {
      struct segment_node *next_segment;

      next_segment = z_container_of(node, struct segment_node, offset_node);
      __segment_offset_tree_detach(self, next_segment);
      __segment_size_tree_detach(self, next_segment);
      __segment_size(closest_segment) += __segment_size(next_segment);

      __segment_free(next_segment);
    }

    __segment_size_tree_attach(self, closest_segment);
  } else {
    struct segment_node *segment;
    segment = __segment_alloc(start, size);
    if (Z_MALLOC_IS_NULL(segment))
      return(2);

    /* TODO: Can we use a bitmap? (7 + 16 + 32 bits avail) */
    __segment_offset_tree_attach(self, segment);
    __segment_size_tree_attach(self, segment);
  }

  self->available += size;
  return(0);
}

int z_space_map_get (z_space_map_t *self, uint32_t size, z_space_segment_t *segment) {
  struct segment_node *closest_segment;
  z_tree_node_t *node;

  node = z_tree_node_closest(self->size_root, __segment_closest_size, &size, self);
  if (Z_UNLIKELY(node == NULL))
    return(1);

  closest_segment = z_container_of(node, struct segment_node, size_node);
  __segment_offset_tree_detach(self, closest_segment);
  __segment_size_tree_detach(self, closest_segment);
  if (__segment_size(closest_segment) > size) {
    segment->offset = __segment_start(closest_segment);
    segment->size = size;
    __segment_start(closest_segment) += size;
    __segment_size(closest_segment) -= size;
    __segment_offset_tree_attach(self, closest_segment);
    __segment_size_tree_attach(self, closest_segment);
  } else {
    segment->offset = __segment_start(closest_segment);
    segment->size = __segment_size(closest_segment);
    __segment_free(closest_segment);
  }

  self->available -= size;
  return(0);
}

void z_space_map_dump (const z_space_map_t *self, FILE *stream) {
  const z_tree_node_t *node;
  uint32_t last_offset = 0;
  z_tree_iter_t iter;
  char szbuffer[32];

  fprintf(stderr, "Space Offsets:\n");
  z_tree_iter_init(&iter, self->offset_root);
  node = z_tree_iter_begin(&iter);
  while (node != NULL) {
    const struct segment_node *segment;

    segment = z_container_of(node, struct segment_node, offset_node);
    if (__segment_start(segment) != last_offset) {
      z_human_size(szbuffer, sizeof(szbuffer), __segment_start(segment) - last_offset);
      fprintf(stderr, "    0x%08d 0x%08d (%s used)\n",
              last_offset,
              __segment_start(segment),
              szbuffer);
    }

    z_human_size(szbuffer, sizeof(szbuffer), __segment_size(segment));
    fprintf(stderr, "    0x%08d 0x%08d (%s avail)\n",
            __segment_start(segment),
            __segment_end(segment),
            szbuffer);

    last_offset = __segment_end(segment);
    node = z_tree_iter_next(&iter);
  }

  fprintf(stderr, "Space Sizes:\n");
  z_tree_iter_init(&iter, self->size_root);
  node = z_tree_iter_begin(&iter);
  while (node != NULL) {
    const struct segment_node *segment;

    segment = z_container_of(node, struct segment_node, size_node);
    z_human_size(szbuffer, sizeof(szbuffer), __segment_size(segment));
    fprintf(stderr, "    0x%08d 0x%08d (%s avail)\n",
            __segment_start(segment),
            __segment_end(segment),
            szbuffer);

    node = z_tree_iter_next(&iter);
  }

  z_human_size(szbuffer, sizeof(szbuffer), self->available);
  fprintf(stderr, "Space Available %s (%u)\n\n", szbuffer, self->available);
}