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


size_t z_memshared (const void *a, size_t alen,
                    const void *b, size_t blen)
{
  const unsigned long *ia = (const unsigned long *)a;
  const unsigned long *ib = (const unsigned long *)b;
  register const unsigned char *ca;
  register const unsigned char *cb;
  register size_t n;
  size_t min_length;

  n = min_length = z_min(alen, blen);
  while (n >= sizeof(unsigned long) && *ia == *ib) {
    n -= sizeof(unsigned long);
    ++ia;
    ++ib;
  }

  ca = (const unsigned char *)ia;
  cb = (const unsigned char *)ib;
  switch (n) {
    case 7: if (*ca++ != *cb++) break; --n;
    case 6: if (*ca++ != *cb++) break; --n;
    case 5: if (*ca++ != *cb++) break; --n;
    case 4: if (*ca++ != *cb++) break; --n;
    case 3: if (*ca++ != *cb++) break; --n;
    case 2: if (*ca++ != *cb++) break; --n;
    case 1: if (*ca++ != *cb++) break; --n;
    case 0: break;
    default:
      while (n > 0 && *ca++ == *cb++) --n;
      break;
  }

  return(min_length - n);
}

size_t z_memrshared (const void *a, size_t alen,
                     const void *b, size_t blen)
{
  const unsigned long *ia = (const unsigned long *)(((char *)a) + alen);
  const unsigned long *ib = (const unsigned long *)(((char *)b) + blen);
  const unsigned char *ca;
  const unsigned char *cb;
  size_t n, min_length;

  n = min_length = z_min(alen, blen);
  while (n >= sizeof(unsigned long) && *(ia - 1) == *(ib - 1)) {
    n -= sizeof(unsigned long);
    --ia;
    --ib;
  }

  ca = (const unsigned char *)ia;
  cb = (const unsigned char *)ib;
  switch (n) {
    case 7: if (*--ca != *--cb) break; --n;
    case 6: if (*--ca != *--cb) break; --n;
    case 5: if (*--ca != *--cb) break; --n;
    case 4: if (*--ca != *--cb) break; --n;
    case 3: if (*--ca != *--cb) break; --n;
    case 2: if (*--ca != *--cb) break; --n;
    case 1: if (*--ca != *--cb) break; --n;
    case 0: break;
    default:
      while (n > 0 && *--ca == *--cb) --n;
      break;
  }
  return(min_length - n);
}
