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

#ifndef _Z_MEMPOOL_H_
#define _Z_MEMPOOL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/allocator.h>
#include <zcl/freelist.h>
#include <zcl/locking.h>
#include <zcl/macros.h>

#define Z_MMPOOL_BLOCKS     4

Z_TYPEDEF_STRUCT(z_mempool)

struct z_mempool {
  z_spinlock_t lock;
  uint8_t *blocks[Z_MMPOOL_BLOCKS];
  z_free_list_t free_lists[Z_MMPOOL_BLOCKS];
  size_t size;
};

int     z_mempool_open    (z_mempool_t *mmpool, z_allocator_t *allocator, size_t size);
void    z_mempool_close   (z_mempool_t *mmpool, z_allocator_t *allocator);

void *  z_mempool_alloc   (z_mempool_t *mmpool);
int     z_mempool_free    (z_mempool_t *mmpool, void *ptr);

__Z_END_DECLS__

#endif /* _Z_MEMPOOL_H_ */
