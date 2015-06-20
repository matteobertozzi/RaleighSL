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

#ifndef _Z_SYSTEM_H_
#define _Z_SYSTEM_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define Z_CACHELINE                       64
#define Z_CACHELINE_PAD(size)             (z_align_up(size, Z_CACHELINE) - size)
#define Z_CACHELINE_PAD_FIELDS(a, ...)    Z_CACHELINE_PAD(z_sizeof_sum(a, ##__VA_ARGS__))

#define z_system_cpu_relax()              asm volatile("pause\n": : :"memory")

unsigned int  z_system_processors   (void);

uint64_t      z_system_memory       (void);
uint64_t      z_system_memory_free  (void);
uint64_t      z_system_memory_used  (void);

__Z_END_DECLS__

#endif /* !_Z_SYSTEM_H_ */
