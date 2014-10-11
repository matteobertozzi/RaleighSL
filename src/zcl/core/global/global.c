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

#include <zcl/global.h>

static z_memory_t __global_mem;
static z_memory_t *__global_mem_ptr = NULL;

z_memory_t *z_global_memory (void) {
  if (__global_mem_ptr == NULL) {
    z_memory_open(&__global_mem, NULL);
    __global_mem_ptr = &__global_mem;
  }
  return(__global_mem_ptr);
}
