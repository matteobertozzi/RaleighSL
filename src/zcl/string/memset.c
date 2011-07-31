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

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <zcl/config.h>
#include <zcl/memset.h>

#define __memset(dest, c, pattern, n)                                       \
    do {                                                                    \
        unsigned long *ip = (unsigned long *)dest;                          \
        unsigned char *cp;                                                  \
                                                                            \
        /* Fast set with n >= sizeof(unsigned long) */                      \
        for (; n >= sizeof(unsigned long); n -= sizeof(unsigned long))      \
            *ip++ = pattern;                                                \
                                                                            \
        /* Set the rest of the buffer */                                    \
        cp = (unsigned char *)ip;                                           \
        while (n--)                                                         \
            *cp++ = c;                                                      \
    } while (0)


#define __memset_sized(type, dest, pattern, n)                              \
    do {                                                                    \
        type *p = (type *)dest;                                             \
                                                                            \
        for (; n >= sizeof(type); n -= sizeof(type))                        \
            *p++ = pattern;                                                 \
    } while (0)

/*
 * Mem-Set
 */
void *z_memset (void *dest, int c, unsigned int n) {
#ifndef Z_STRING_HAS_MEMSET
    unsigned long pattern = (unsigned long)c;

    switch (sizeof(unsigned long)) {
        case 4:
            pattern |= ((uint32_t)c) <<  8;
            pattern |= ((uint32_t)c) << 16;
            pattern |= ((uint32_t)c) << 24;
            __memset(dest, c, pattern, n);
            break;
        case 8:
            pattern |= ((uint64_t)c) <<  8;
            pattern |= ((uint64_t)c) << 16;
            pattern |= ((uint64_t)c) << 24;
            pattern |= ((uint64_t)c) << 32;
            pattern |= ((uint64_t)c) << 40;
            pattern |= ((uint64_t)c) << 48;
            pattern |= ((uint64_t)c) << 56;
            __memset(dest, c, pattern, n);
            break;
        default:
            __memset_sized(uint8_t, dest, c, n);
            break;
    }

    return(dest);
#else
    return(memset(dest, c, n));
#endif /* !Z_STRING_HAS_MEMSET */
}

void *z_memset8 (void *dest, uint8_t c, unsigned int n) {
    __memset_sized(uint8_t, dest, c, n);
    return(dest);
}

void *z_memset16 (void *dest, uint8_t c, unsigned int n) {
    uint16_t pattern = (uint16_t)c;
    pattern |= ((uint32_t)c) <<  8;
    __memset_sized(uint16_t, dest, c, n);
    return(dest);
}

void *z_memset32 (void *dest, uint8_t c, unsigned int n) {
    uint32_t pattern = (uint32_t)c;
    pattern |= ((uint32_t)c) <<  8;
    pattern |= ((uint32_t)c) << 16;
    pattern |= ((uint32_t)c) << 24;
    __memset_sized(uint32_t, dest, pattern, n);
    return(dest);
}

void *z_memset64 (void *dest, uint8_t c, unsigned int n) {
    uint64_t pattern = (uint64_t)c;
    pattern |= ((uint64_t)c) <<  8;
    pattern |= ((uint64_t)c) << 16;
    pattern |= ((uint64_t)c) << 24;
    pattern |= ((uint64_t)c) << 32;
    pattern |= ((uint64_t)c) << 40;
    pattern |= ((uint64_t)c) << 48;
    pattern |= ((uint64_t)c) << 56;
    __memset_sized(uint64_t, dest, pattern, n);
    return(dest);
}

/*
 * Mem set Pattern
 */
void *z_memset_p8 (void *dest, uint8_t pattern, unsigned int n) {
    __memset_sized(uint8_t, dest, pattern, n);
    return(dest);
}

void *z_memset_p16 (void *dest, uint16_t pattern, unsigned int n) {
    __memset_sized(uint16_t, dest, pattern, n);
    return(dest);
}

void *z_memset_p32 (void *dest, uint32_t pattern, unsigned int n) {
    __memset_sized(uint32_t, dest, pattern, n);
    return(dest);
}

void *z_memset_p64 (void *dest, uint64_t pattern, unsigned int n) {
    __memset_sized(uint64_t, dest, pattern, n);
    return(dest);
}

/*
 * Mem-Zero
 */
void *z_memzero (void *dest, unsigned int n) {
#ifndef Z_STRING_HAS_MEMSET
    __memset(dest, 0U, 0U, n);
    return(dest);
#else
    return(memset(dest, 0, n));
#endif /* Z_STRING_HAS_MEMSET */
}

void *z_memzero8 (void *dest, unsigned int n) {
    uint8_t z8 = 0U;
    __memset_sized(uint8_t, dest, z8, n);
    return(dest);
}

void *z_memzero16 (void *dest, unsigned int n) {
    uint16_t z16 = 0U;
    __memset_sized(uint16_t, dest, z16, n);
    return(dest);
}

void *z_memzero32 (void *dest, unsigned int n) {
    uint32_t z32 = 0U;
    __memset_sized(uint32_t, dest, z32, n);
    return(dest);
}

void *z_memzero64 (void *dest, unsigned int n) {
    uint64_t z64 = 0U;
    __memset_sized(uint64_t, dest, z64, n);
    return(dest);
}
