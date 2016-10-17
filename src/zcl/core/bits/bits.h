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

#ifndef _Z_BITS_H_
#define _Z_BITS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define z_rotl(x, r, s)                 (((x) << (r)) | ((x) >> ((s) - (r))))
#define z_rotr(x, r, s)                 (((x) >> (r)) | ((x) << ((s) - (r))))
#define z_rotl32(x, r)                  z_rotl(x, r, 32)
#define z_rotl64(x, r)                  z_rotl(x, r, 64)
#define z_rotr32(x, r)                  z_rotr(x, r, 32)
#define z_rotr64(x, r)                  z_rotr(x, r, 64)

#define z_shl56(x)                      (((uint64_t)(x)) << 56)
#define z_shl48(x)                      (((uint64_t)(x)) << 48)
#define z_shl40(x)                      (((uint64_t)(x)) << 40)
#define z_shl32(x)                      (((uint64_t)(x)) << 32)
#define z_shl24(x)                      (((uint64_t)(x)) << 24)
#define z_shl16(x)                      ((x) << 16)
#define z_shl8(x)                       ((x) <<  8)

#define z_clz8(x)                       z_clz32(x)
#define z_clz16(x)                      z_clz32(x)
#define z_clz32(x)                      __builtin_clz(x)
#define z_clz64(x)                      __builtin_clzl(x)

__Z_END_DECLS__

#endif /* !_Z_BITS_H_ */
