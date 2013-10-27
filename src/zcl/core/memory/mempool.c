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

#include <zcl/mempool.h>
#include <zcl/debug.h>

#define Z_MEMPOOL_BLOCK_SIZE          (64 * 1024)

int z_mempool_open (z_mempool_t *mmpool, z_allocator_t *allocator, size_t size) {
  int i;
  mmpool->size = size;
  z_spin_alloc(&(mmpool->lock));
  for (i = 0; i < Z_MMPOOL_BLOCKS; ++i) {
    mmpool->blocks[i] = z_allocator_alloc(allocator, uint8_t, Z_MEMPOOL_BLOCK_SIZE);
    Z_ASSERT(mmpool->blocks[i] != NULL, "Unable to allocate mmpool %u", size);
    z_free_list_init(&(mmpool->free_lists[i]));
  }
  return(0);
}

void z_mempool_close (z_mempool_t *mmpool, z_allocator_t *allocator) {
  int i;
  for (i = 0; i < Z_MMPOOL_BLOCKS; ++i) {
    z_allocator_free(allocator, mmpool->blocks[i]);
  }
  z_spin_free(&(mmpool->lock));
}

void *z_mempool_alloc (z_mempool_t *mmpool) {
  void *ptr = NULL;
  z_lock(&(mmpool->lock), z_spin, {
    int i;
    for (i = 0; i < Z_MMPOOL_BLOCKS; ++i) {
      if ((ptr = z_free_list_pop(&(mmpool->free_lists[i]))) != NULL)
        break;
    }
  });
  return(ptr);
}

int z_mempool_free (z_mempool_t *mmpool, void *ptr) {
  uint8_t *uptr = (uint8_t *)ptr;
  int freed = 0;
  z_lock(&(mmpool->lock), z_spin, {
    int i;
    for (i = 0; i < Z_MMPOOL_BLOCKS; ++i) {
      const uint8_t *blk_start = mmpool->blocks[i];
      const uint8_t *blk_end = blk_start + mmpool->size;
      freed = (uptr >= blk_start && uptr <= blk_end);
      if (freed) {
        z_free_list_add(&(mmpool->free_lists[i]), ptr);
        break;
      }
    }
  });
  return(!freed);
}
