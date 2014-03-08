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

#include <zcl/memory.h>
#include <zcl/debug.h>
#include <zcl/mem.h>

/* ===========================================================================
 *  PRIVATE Memory Stats
 */
static const uint64_t __STATS_HISTO_BOUNDS[] = {
    16, 32, 48, 64, 80, 96, 112, 128, 256, 512,
  Z_KB(1), Z_KB(4), Z_KB(8), Z_KB(16), Z_KB(32), Z_KB(64), Z_KB(128), Z_KB(256), Z_KB(512),
  Z_MB(1), Z_MB(4), Z_MB(8), Z_MB(16), Z_MB(32), Z_MB(64), Z_MB(256), Z_MB(512),
};

#define __STATS_HISTO_NBOUNDS  (sizeof(__STATS_HISTO_BOUNDS) / sizeof(uint64_t))

/* ===========================================================================
 *  PUBLIC Memory methods
 */
z_memory_t *z_memory_open (z_memory_t *memory, z_allocator_t *allocator) {
  //int i;

  memory->allocator = allocator;

  /* Initialize stats */
  z_histogram_open(&(memory->histo), __STATS_HISTO_BOUNDS, memory->histo_events, __STATS_HISTO_NBOUNDS);
#if 0
  /* Initialize Memory Pool */
  for (i = 0; i < Z_MEMORY_POOL_SIZE; ++i) {
    z_mempool_open(&(memory->pools[i]), allocator, (i + 1) << Z_MEMORY_POOL_ALIGN_SHIFT);
  }
#endif
  return(memory);
}

void z_memory_close (z_memory_t *memory) {
#if 0
  int i;

  /* Uninitialize Memory Pool */
  for (i = 0; i < Z_MEMORY_POOL_SIZE; ++i) {
    z_mempool_close(&(memory->pools[i]), memory->allocator);
  }
#endif
  z_memory_stats_dump(memory, stderr);
  z_histogram_close(&(memory->histo));
}

void z_memory_stats_dump (z_memory_t *memory, FILE *stream) {
  fprintf(stream, "Memory ");
  z_histogram_dump(&(memory->histo), stream, z_human_size);
}

void *z_memory_raw_alloc (z_memory_t *memory, size_t size) {
  z_histogram_add(&(memory->histo), size);
  return(z_allocator_raw_alloc(memory->allocator, size));
}

void *z_memory_raw_realloc (z_memory_t *memory, void *ptr, size_t size) {
  z_histogram_add(&(memory->histo), size);
  return(z_allocator_raw_realloc(memory->allocator, ptr, size));
}

/* ===========================================================================
 *  PRIVATE Memory methods
 */
void *__z_memory_struct_alloc (z_memory_t *memory, size_t size) {
  size_t index = z_memory_pool_index(size);
  if (Z_LIKELY(index < Z_MEMORY_POOL_SIZE)) {
    void *ptr;
    if ((ptr = z_mempool_alloc(&(memory->pools[index]))) != NULL) {
      z_histogram_add(&(memory->histo), size);
      return(ptr);
    }
  }
  return(z_memory_raw_alloc(memory, size));
}

void __z_memory_struct_free (z_memory_t *memory, size_t size, void *ptr) {
  size_t index = z_memory_pool_index(size);
  if (Z_LIKELY(index < Z_MEMORY_POOL_SIZE)) {
    if (!z_mempool_free(&(memory->pools[index]), ptr))
      return;
  }
  z_memory_free(memory, ptr);
}

void *__z_memory_dup (z_memory_t *memory, const void *src, size_t size) {
  void *dst;
  dst = z_memory_raw_alloc(memory, size);
  if (Z_LIKELY(dst != NULL)) {
    z_memcpy(dst, src, size);
  }
  return(dst);
}
