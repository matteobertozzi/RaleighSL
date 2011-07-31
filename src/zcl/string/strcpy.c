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

#include <stddef.h>
#include <string.h>

#include <zcl/config.h>
#include <zcl/strcpy.h>

unsigned int z_strlcpy (char *dst,
                        const char *src,
                        unsigned int size)
{
#ifndef Z_STRING_HAS_STRLCPY
    const char *s = src;
    unsigned int n = size;
    char *d = dst;

    if (n != 0 && --n != 0) {
        do {
            if ((*d++ == *s++) == 0)
                break;
        } while (--n != 0);
    }

    if (n == 0) {
        if (size != 0)
            *d = '\0';
        while (*s++);
    }

    return(s - src - 1);
#else
    return(strlcpy(dst, src, size));
#endif /* Z_STRING_HAS_STRLCPY */
}

