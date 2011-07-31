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
#include <stdint.h>

#include <zcl/config.h>
#include <zcl/strlen.h>

unsigned int z_strlen8 (const char *str) {
    register unsigned int length = 0;

    while (*str++ != 0)
        length++;

    return(length);
}

unsigned int z_strlen16 (const char *str) {
    register unsigned int length = 0U;
    register const uint16_t *p;

    while ((((unsigned long)str) & (sizeof(uint16_t) - 1)) != 0) {
        if (*str++ == 0)
            return(length);
        length++;
    }

    p = (const uint16_t *)str;
    for (;;) {
        if ((*p - 0x0101U) & ~(*p) & 0x8080U) {
            if (!(*p & 0x00ffU)) return(length);
            return(length + 1); /* if (!(*p & 0xff00U)) */
        }
        length += sizeof(uint16_t);
        p++;
    }

    return(length);
}

unsigned int z_strlen32 (const char *str) {
    register unsigned int length = 0U;
    register const uint32_t *p;

    while ((((unsigned long)str) & (sizeof(uint32_t) - 1)) != 0) {
        if (*str++ == 0)
            return(length);
        length++;
    }

    p = (const uint32_t *)str;
    for (;;) {
        if ((*p - 0x01010101U) & ~(*p) & 0x80808080U) {
            if (!(*p & 0x000000ffU)) return(length);
            if (!(*p & 0x0000ff00U)) return(length + 1U);
            if (!(*p & 0x00ff0000U)) return(length + 2U);
            return(length + 3U); /* if (!(*p & 0xff000000U)) */
        }
        length += sizeof(uint32_t);
        p++;
    }

    return(length);
}

unsigned int z_strlen64 (const char *str) {
    register unsigned int length = 0U;
    register const uint64_t *p;

    while ((((long)str) & (sizeof(uint64_t) - 1)) != 0) {
        if (*str++ == 0)
            return(length);
        length++;
    }

    p = (const uint64_t *)str;
    for (;;) {
        if ((*p - 0x0101010101010101ULL) & ~(*p) & 0x8080808080808080ULL) {
            if (!(*p & 0x00000000000000ffULL)) return(length);
            if (!(*p & 0x000000000000ff00ULL)) return(length + 1U);
            if (!(*p & 0x0000000000ff0000ULL)) return(length + 2U);
            if (!(*p & 0x00000000ff000000ULL)) return(length + 3U);
            if (!(*p & 0x000000ff00000000ULL)) return(length + 4U);
            if (!(*p & 0x0000ff0000000000ULL)) return(length + 5U);
            if (!(*p & 0x00ff000000000000ULL)) return(length + 6U);
            return(length + 7U); /* if (!(*p & 0xff00000000000000)) */
        }
        length += sizeof(uint64_t);
        p++;
    }

    return(length);
}

unsigned int z_strlen (const char *str) {
    switch (sizeof(unsigned long)) {
        case 8: return(z_strlen64(str));
        case 4: return(z_strlen32(str));
        case 2: return(z_strlen16(str));
    }
    return(z_strlen8(str));
}

