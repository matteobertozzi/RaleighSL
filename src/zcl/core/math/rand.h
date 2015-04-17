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

#ifndef _Z_RAND_H_
#define _Z_RAND_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

uint32_t z_rand32     (uint64_t *seed);
uint64_t z_rand64     (uint64_t *seed);

uint32_t z_rand32_bounded (uint64_t *seed, uint32_t vmin, uint32_t vmax);
uint64_t z_rand64_bounded (uint64_t *seed, uint64_t vmin, uint64_t vmax);

void     z_rand_bytes (uint64_t *seed, uint8_t *bytes, size_t length);
void     z_rand_uuid  (uint64_t *seed, uint8_t uuid[16]);

__Z_END_DECLS__

#endif /* _Z_RAND_H_ */
