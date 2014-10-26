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

#ifndef _Z_KEY_CMP_H_
#define _Z_KEY_CMP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

int z_keycmp_u32   (void *udata, const void *a, const void *b);
int z_keycmp_u64   (void *udata, const void *a, const void *b);

int z_keycmp_mm32  (void *udata, const void *a, const void *b);
int z_keycmp_mm64  (void *udata, const void *a, const void *b);
int z_keycmp_mm128 (void *udata, const void *a, const void *b);
int z_keycmp_mm256 (void *udata, const void *a, const void *b);
int z_keycmp_mm512 (void *udata, const void *a, const void *b);

__Z_END_DECLS__

#endif /* !_Z_KEY_CMP_H_ */
