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

#ifndef _Z_SEGMENT_MAP_H_
#define _Z_SEGMENT_MAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/tree.h>

Z_TYPEDEF_STRUCT(z_space_segment)
Z_TYPEDEF_STRUCT(z_space_map)

struct z_space_segment {
  uint32_t offset;
  uint32_t size;
};

struct z_space_map {
  z_tree_node_t *offset_root;
  z_tree_node_t *size_root;
  uint32_t available;
};

void z_space_map_open   (z_space_map_t *self);
void z_space_map_close  (z_space_map_t *self);
int  z_space_map_add    (z_space_map_t *self, uint32_t offset, uint32_t size);
int  z_space_map_get    (z_space_map_t *self, uint32_t size, z_space_segment_t *segment);
void z_space_map_dump   (const z_space_map_t *self, FILE *stream);

__Z_END_DECLS__

#endif /* _Z_SEGMENT_MAP_H_ */
