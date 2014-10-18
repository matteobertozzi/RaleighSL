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

#ifndef _Z_MEMUTIL_H_
#define _Z_MEMUTIL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#include <string.h>

#define z_memchr              memchr
#define z_memmem              memmem
#define z_memmove             memmove
#define z_memcpy              memcpy
#define z_memset              memset
#define z_memcmp              memcmp
#define z_memzero(dst, n)     z_memset(dst, 0, n)
#define z_memeq(m1, m2, n)    (!z_memcmp(m1, m2, n))


#define z_mem_equals(a, asize, b, bsize)                          \
  (!z_mem_compare(a, asize, b, bsize))

#define z_mem_compare(a, asize, b, bsize) ({                      \
  int __cmp;                                                      \
  if (asize == bsize) {                                           \
    __cmp = z_memcmp(a, b, asize);                                \
  } else {                                                        \
    __cmp = z_memcmp(a, b, z_min(asize, bsize));                  \
    __cmp = (__cmp != 0) ? __cmp : (asize - bsize);               \
  }                                                               \
  __cmp;                                                          \
})

#define z_memslice_overlap(a_start, a_end, b_start, b_end) ({     \
  int __cmp1, __cmp2;                                             \
  __cmp1 = z_memslice_compare(a_start, b_end);                    \
  /* can we break here if cmp1 > 0? */                            \
  __cmp2 = z_memslice_compare(b_start, a_end);                    \
  ((cmp2 > 0) - (cmp1 > 0));                                      \
})

void    z_memswap     (void *a, void *b, size_t n);
void    z_memswap8    (void *a, void *b, size_t n);
void    z_memswap16   (void *a, void *b, size_t n);
void    z_memswap32   (void *a, void *b, size_t n);
void    z_memswap64   (void *a, void *b, size_t n);

size_t  z_memshared   (const void *a, size_t alen,
                       const void *b, size_t blen);
size_t  z_memrshared  (const void *a, size_t alen,
                       const void *b, size_t blen);

#define z_memcpy16(dst, src, len)                             \
  do {                                                        \
    register const uint8_t *psrc = (const uint8_t *)(src);    \
    register uint8_t *pdst = (uint8_t *)(dst);                \
    switch (len) {                                            \
      case 2: pdst[1] = psrc[1];                              \
      case 1: pdst[0] = psrc[0];                              \
    }                                                         \
  } while (0)

#define z_memcpy32(dst, src, len)                             \
  do {                                                        \
    register const uint8_t *psrc = (const uint8_t *)(src);    \
    register uint8_t *pdst = (uint8_t *)(dst);                \
    switch (len) {                                            \
      case 4: pdst[3] = psrc[3];                              \
      case 3: pdst[2] = psrc[2];                              \
      case 2: pdst[1] = psrc[1];                              \
      case 1: pdst[0] = psrc[0];                              \
    }                                                         \
  } while (0)

#define z_memcpy64(dst, src, len)                             \
  do {                                                        \
    register const uint8_t *psrc = (const uint8_t *)(src);    \
    register uint8_t *pdst = (uint8_t *)(dst);                \
    switch (len) {                                            \
      case 8: pdst[7] = psrc[7];                              \
      case 7: pdst[6] = psrc[6];                              \
      case 6: pdst[5] = psrc[5];                              \
      case 5: pdst[4] = psrc[4];                              \
      case 4: pdst[3] = psrc[3];                              \
      case 3: pdst[2] = psrc[2];                              \
      case 2: pdst[1] = psrc[1];                              \
      case 1: pdst[0] = psrc[0];                              \
    }                                                         \
  } while (0)

__Z_END_DECLS__

#endif /* !_Z_MEMUTIL_H_ */
