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

#include <string.h>
#include <ctype.h>

#include <zcl/config.h>
#include <zcl/memcmp.h>
#include <zcl/strlen.h>
#include <zcl/strcmp.h>

#define __tolower(c)     ((!isalpha(c) || islower(c)) ? (c) : ((c) - 'A' + 'a'))

int z_strncmp (const char *s1, const char *s2, unsigned int n) {
#ifndef Z_STRING_HAS_STRNCMP
    unsigned int l1, l2;
    unsigned int cmp;

    if ((l1 = z_strlen(s1)) < n) n = l1;
    if ((l2 = z_strlen(s2)) < n) n = l2;

    if ((cmp = z_memcmp(s1, s2, n)))
        return(cmp);

    return(l1 - l2);
#else
    return(strncmp(s1, s2, n));
#endif
}

int z_strcmp (const char *s1, const char *s2) {
#ifndef Z_STRING_HAS_STRCMP
    unsigned int l1, l2;
    int cmp;

    l1 = z_strlen(s1);
    l2 = z_strlen(s2);

    if ((cmp = z_memcmp(s1, s2, l1 < l2 ? l1 : l2)))
        return(cmp);

    return(l1 - l2);
#else
    return(strcmp(s1, s2));
#endif
}

int z_strncasecmp (const char *s1, const char *s2, unsigned int n) {
    unsigned char c1, c2;

    while (n--) {
        c1 = *s1++;
        c2 = *s2++;
        if ((c1 = __tolower(c1)) != (c2 = __tolower(c2)))
            return(c1 - c2);

        if (!c1)
            break;
    };

    return(0);
}

int z_strcasecmp (const char *s1, const char *s2) {
    register unsigned char c1, c2;

    do {
        c1 = *s1++;
        c2 = *s2++;
        if ((c1 = __tolower(c1)) != (c2 = __tolower(c2)))
            return(c1 - c2);

    } while (c1);

    return(0);
}

