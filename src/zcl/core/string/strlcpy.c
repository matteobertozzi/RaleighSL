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

#include <zcl/config.h>
#ifndef Z_STRING_HAS_STRLCPY

#include <zcl/memutil.h>
#include <zcl/string.h>

size_t z_strlcpy (char *dst, const char *src, size_t size) {
  size_t n = z_strlen(src);

  if (Z_LIKELY(size)) {
    size_t len = (n >= size) ? (size - 1) : n;
    z_memcpy(dst, src, len);
    dst[len] = '\0';
  }

  return(n);
}

#endif /* !Z_STRING_HAS_STRLCPY */
