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
#include <zcl/humans.h>
#include <zcl/debug.h>

/* ===========================================================================
 *  PRIVATE Memory Stats
 */
#define __STATS_HISTO_NBOUNDS       z_fix_array_size(__STATS_HISTO_BOUNDS)
static const uint64_t __STATS_HISTO_BOUNDS[28] = {
    16, 32, 48, 64, 80, 96, 112, 128, 256, 512,
  Z_KIB(1),   Z_KIB(2),   Z_KIB(4),   Z_KIB(8),   Z_KIB(16),
  Z_KIB(32),  Z_KIB(64),  Z_KIB(128), Z_KIB(256), Z_KIB(512),
  Z_MIB(1),   Z_MIB(8),   Z_MIB(16),  Z_MIB(32),  Z_MIB(64),
  Z_MIB(128), Z_MIB(256), 0xffffffffffffffffll,
};

/* ===========================================================================
 *  PUBLIC Memory methods
 */
z_memory_t *z_memory_open (z_memory_t *self, z_allocator_t *allocator) {
  self->allocator = allocator;

  /* Initialize stats */
  z_histogram_init(&(self->histo), __STATS_HISTO_BOUNDS,
                   self->histo_events, __STATS_HISTO_NBOUNDS);

  return(self);
}


void z_memory_close (z_memory_t *memory) {
}

void z_memory_stats_dump (z_memory_t *self, FILE *stream) {
  fprintf(stream, "Memory ");
  z_histogram_dump(&(self->histo), stream, z_human_size_d);
}

/* ===========================================================================
 *  PUBLIC Memory methods
 */
#if Z_MALLOC_HAS_USABLE_SIZE
#include <malloc.h>
#endif

void *z_memory_raw_alloc (z_memory_t *self, const char *type_name, size_t size) {
  void *ptr;
  ptr = z_allocator_raw_alloc(self->allocator, size);
  z_histogram_add(&(self->histo), size);
#if __Z_DEBUG__
  #if Z_MALLOC_HAS_USABLE_SIZE
    Z_LOG_INFO("[MALLOC] %zu %s (phy %zu - diff %ld) %p", size, type_name,
              malloc_usable_size(ptr), malloc_usable_size(ptr) - size, ptr);
  #else
    Z_LOG_TRACE("[MALLOC] %zu %s %p", size, type_name, ptr);
  #endif
  z_memory_set_dirty_debug(ptr, size);
#endif
  return(ptr);
}

void z_memory_raw_free (z_memory_t *self, const char *type_name, void *ptr, size_t size) {
#if __Z_DEBUG__
  Z_LOG_TRACE("[FREE] %zu %s %p", size, type_name, ptr);
#endif
  z_allocator_free(self->allocator, ptr, size);
}
