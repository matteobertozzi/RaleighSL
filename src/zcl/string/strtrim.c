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

#include <ctype.h>

#include <zcl/strtrim.h>
#include <zcl/strlen.h>
#include <zcl/memcpy.h>

unsigned int z_strnltrim (char *str, unsigned int n) {
    unsigned int start = 0U;

    while (isspace(str[start]) && start < n)
        start++;

    if (start > 0U) {
        z_memmove(str, str + start, n - start);
        str[n - start] = '\0';
    }

    return(n);
}

unsigned int z_strnrtrim (char *str, unsigned int n) {
    while (n > 0U && isspace(str[n - 1]))
        n--;

    str[n] = '\0';
    return(n);
}

unsigned int z_strntrim (char *str, unsigned int n) {
    n = z_strnrtrim(str, n);
    n = z_strnltrim(str, n);
    return(n);
}

char *z_strltrim (char *str) {
    z_strnltrim(str, z_strlen(str));
    return(str);
}

char *z_strrtrim (char *str) {
    z_strnrtrim(str, z_strlen(str));
    return(str);
}

char *z_strtrim (char *str) {
    z_strntrim(str, z_strlen(str));
    return(str);
}

