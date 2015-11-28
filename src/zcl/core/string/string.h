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

#ifndef _Z_STRING_H_
#define _Z_STRING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#include <string.h>

#define z_strlen              strlen
#define z_strcmp              strcmp
#define z_strncmp             strncmp

#ifdef Z_STRING_HAS_STRLCPY
  #define z_strlcpy           strlcpy
#else
  size_t  z_strlcpy (char *dst, const char *src, size_t size);
#endif

char *z_strupper    (char *str);
char *z_strnupper   (char *str, size_t n);

char *z_strlower    (char *str);
char *z_strnlower   (char *str, size_t n);

int   z_strcasecmp  (const char *s1, const char *s2);
int   z_strncasecmp (const char *s1, const char *s2, size_t n);

__Z_END_DECLS__

#endif /* !_Z_STRING_H_ */
