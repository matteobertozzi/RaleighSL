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

#ifndef _Z_DATA_MEM_H_
#define _Z_DATA_MEM_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <string.h>

#include <zcl/macros.h>

#define z_memchr              memchr
#define z_memmem              memmem
#define z_memmove             memmove
#define z_memcpy              memcpy
#define z_memset              memset
#define z_memcmp              memcmp
#define z_memzero(dst, n)     z_memset(dst, 0, n)
#define z_memeq(m1, m2, n)    (!z_memcmp(m1, m2, n))

size_t      z_memshared     (const void *a,
                             size_t alen,
                             const void *b,
                             size_t blen);
size_t      z_memrshared    (const void *a,
                             size_t alen,
                             const void *b,
                             size_t blen);

__Z_END_DECLS__

#endif /* _Z_DATA_MEM_H_ */
