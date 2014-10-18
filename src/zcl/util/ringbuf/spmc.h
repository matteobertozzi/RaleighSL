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

#ifndef _Z_SPMC_H_
#define _Z_SPMC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

int       z_spmc_init            (uint8_t *block, uint32_t size, uint64_t ringsize);
uint64_t *z_spmc_add_consumer    (uint8_t *block);
int       z_spmc_remove_consumer (uint8_t *block, uint64_t *seqid);
int       z_spmc_try_next        (uint8_t *block, uint64_t *seqid);
void      z_spmc_publish         (uint8_t *block, uint64_t seqid);
uint64_t  z_spmc_last_publishd   (const uint8_t *block);

__Z_END_DECLS__

#endif /* _Z_SPMC_H_ */
