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

#ifndef _Z_CHMAP_H_
#define _Z_CHMAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memory.h>

Z_TYPEDEF_STRUCT(z_chmap_entry);
Z_TYPEDEF_STRUCT(z_chmap);

struct z_chmap_entry {
  z_chmap_entry_t *hash;
  uint64_t oid;
  uint32_t refs;
  uint8_t  ustate;
  uint8_t  uflags8;
  uint16_t uweight;
};

void             z_chmap_entry_init   (z_chmap_entry_t *entry, uint64_t oid);

z_chmap_t *      z_chmap_create       (z_memory_t *memory, uint32_t size);
void             z_chmap_destroy      (z_memory_t *memory, z_chmap_t *map);
z_chmap_entry_t *z_chmap_try_insert   (z_chmap_t *map, z_chmap_entry_t *entry);
z_chmap_entry_t *z_chmap_remove       (z_chmap_t *map, uint64_t oid);
int              z_chmap_remove_entry (z_chmap_t *map, z_chmap_entry_t *entry);
z_chmap_entry_t *z_chmap_lookup       (z_chmap_t *map, uint64_t oid);

__Z_END_DECLS__

#endif /* !_Z_CHMAP_H_ */
