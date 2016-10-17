/*
 *   Copyright 2011 Matteo Bertozzi
 *
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

#include <zcl/memutil.h>

#define __memswap_sized(type, a, b, n)                   \
  do {                                                   \
    type *ia = (type *)a;                                \
    type *ib = (type *)b;                                \
    type it;                                             \
    for (; n >= sizeof(type); n -= sizeof(type)) {       \
      it = *ia;                                          \
      *ia++ = *ib;                                       \
      *ib++ = it;                                        \
    }                                                    \
  } while (0)

void z_memswap (void *a, void *b, size_t n) {
  unsigned long *ia = (unsigned long *)a;
  unsigned long *ib = (unsigned long *)b;
  unsigned char *ca;
  unsigned char *cb;
  unsigned long it;
  unsigned char ct;

  /* Fast copy while n >= sizeof(unsigned long) */
  for (; n >= sizeof(unsigned long); n -= sizeof(unsigned long)) {
    it = *ia;
    *ia++ = *ib;
    *ib++ = it;
  }

  /* Copy the rest of the data */
  ca = (unsigned char *)ia;
  cb = (unsigned char *)ib;
  while (n--) {
    ct = *ca;
    *ca++ = *cb;
    *cb++ = ct;
  }
}

void z_memswap8 (void *a, void *b, size_t n) {
  __memswap_sized(uint8_t, a, b, n);
}

void z_memswap16 (void *a, void *b, size_t n) {
  __memswap_sized(uint16_t, a, b, n);
}

void z_memswap32 (void *a, void *b, size_t n) {
  __memswap_sized(uint32_t, a, b, n);
}

void z_memswap64 (void *a, void *b, size_t n) {
  __memswap_sized(uint64_t, a, b, n);
}
