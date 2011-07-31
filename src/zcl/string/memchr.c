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
#include <string.h>

#include <zcl/config.h>
#include <zcl/memchr.h>

#define __mask16(x)      (((x) << 8) + (x))
#define __mask32(x)      (((x) << 24) + ((x) << 16) + __mask16(x))
#define __mask48(x)      (((x) << 40) + ((x) << 32) + __mask32(x))
#define __mask64(x)      (((x) << 56) + ((x) << 48) + __mask48(x))

void *z_memchr8 (const void *s, uint8_t c, unsigned int n) {
    register const uint8_t *p = (const uint8_t *)s;

    while (n--) {
        if (*p == c)
            return((void *)p);
        p++;
    }

    return(NULL);
}

void *z_memchr16 (const void *s, uint8_t c, unsigned int n) {
    register const uint16_t *ip = (const uint16_t *)s;
    register const uint8_t *p;
    register uint16_t mask;
    register uint16_t x;

    mask = __mask16((uint16_t)c);
    while (n >= sizeof(uint16_t)) {
        x = *ip ^ mask;
        if ((x - 0x0101U) & ~x & 0x8080U)
            break;

        n -= sizeof(uint16_t);
        ip++;
    }

    p = (const uint8_t *)ip;
    while (n--) {
        if (*p == c)
            return((void *)p);
        p++;
    }

    return(NULL);
}

void *z_memchr32 (const void *s, uint8_t c, unsigned int n) {
    register const uint32_t *ip = (const uint32_t *)s;
    register const uint8_t *p;
    register uint32_t mask;
    register uint32_t x;

    mask = __mask32((uint32_t)c);
    while (n >= sizeof(uint32_t)) {
        x = *ip ^ mask;
        if ((x - 0x01010101U) & ~x & 0x80808080U)
            break;

        n -= sizeof(uint32_t);
        ip++;
    }

    p = (const uint8_t *)ip;
    while (n--) {
        if (*p == c)
            return((void *)p);
        p++;
    }

    return(NULL);
}

void *z_memchr64 (const void *s, uint8_t c, unsigned int n) {
    register const uint64_t *ip = (const uint64_t *)s;
    register const uint8_t *p;
    register uint64_t mask;
    register uint64_t x;

    mask = __mask64((uint64_t)c);
    while (n >= sizeof(uint64_t)) {
        x = *ip ^ mask;
        if ((x - 0x0101010101010101ULL) & ~(x) & 0x8080808080808080ULL)
            break;

        n -= sizeof(uint64_t);
        ip++;
    }

    p = (const uint8_t *)ip;
    while (n--) {
        if (*p == c)
            return((void *)p);
        p++;
    }

    return(NULL);
}

void *z_memchr (const void *s, uint8_t c, unsigned int n) {
#ifndef Z_STRING_HAS_MEMCHR
    switch ((n < sizeof(unsigned long)) ? n : sizeof(unsigned long)) {
        case 8: return(z_memchr64(s, c, n));
        case 4: return(z_memchr32(s, c, n));
        case 2: return(z_memchr16(s, c, n));
    }
    return(z_memchr8(s, c, n));
#else
    return(memchr(s, c, n));
#endif /* !Z_STRING_HAS_MEMCHR */
}

void *z_memrchr8 (const void *s, uint8_t c, unsigned int n) {
    register const uint8_t *p = (const uint8_t *)s;

    while (n--) {
        if (p[n] == c)
            return((void *)(p + n));
    }

    return(NULL);
}

void *z_memrchr16 (const void *s, uint8_t c, unsigned int n) {
    register const uint16_t *ip;
    register const uint8_t *p;
    register uint16_t mask;
    register uint16_t x;

    ip = (const uint16_t *)((const uint8_t *)s + n - 1 - sizeof(uint16_t));
    mask = __mask16((uint16_t)c);
    while (n >= sizeof(uint16_t)) {
        x = *ip ^ mask;
        if ((x - 0x0101U) & ~x & 0x8080U)
            break;

        n -= sizeof(uint16_t);
        ip--;
    }

    p = (const uint8_t *)ip;
    while (n--) {
        if (p[n] == c)
            return((void *)(p + n));
    }

    return(NULL);
}

void *z_memrchr32 (const void *s, uint8_t c, unsigned int n) {
    register const uint32_t *ip;
    register const uint8_t *p;
    register uint32_t mask;
    register uint32_t x;

    ip = (const uint32_t *)((const uint8_t *)s + n - 1 - sizeof(uint32_t));
    mask = __mask32((uint32_t)c);
    while (n >= sizeof(uint32_t)) {
        x = *ip ^ mask;
        if ((x - 0x01010101U) & ~x & 0x80808080U)
            break;

        n -= sizeof(uint32_t);
        ip--;
    }

    p = (const uint8_t *)ip;
    while (n--) {
        if (p[n] == c)
            return((void *)(p + n));
    }

    return(NULL);
}

void *z_memrchr64 (const void *s, uint8_t c, unsigned int n) {
    register const uint64_t *ip;
    register const uint8_t *p;
    register uint64_t mask;
    register uint64_t x;

    ip = (const uint64_t *)((const uint8_t *)s + n - 1 - sizeof(uint64_t));
    mask = __mask64((uint64_t)c);
    while (n >= sizeof(uint64_t)) {
        x = *ip ^ mask;
        if ((x - 0x0101010101010101ULL) & ~(x) & 0x8080808080808080ULL)
            break;

        n -= sizeof(uint64_t);
        ip--;
    }

    p = (const uint8_t *)ip;
    while (n--) {
        if (p[n] == c)
            return((void *)(p + n));
    }

    return(NULL);
}

void *z_memrchr (const void *s, uint8_t c, unsigned int n) {
#ifndef Z_STRING_HAS_MEMRCHR
    switch ((n < sizeof(unsigned long)) ? n : sizeof(unsigned long)) {
        case 8: return(z_memrchr64(s, c, n));
        case 4: return(z_memrchr32(s, c, n));
        case 2: return(z_memrchr16(s, c, n));
    }
    return(z_memrchr8(s, c, n));
#else
    return(memrchr(s, c, n));
#endif /* !Z_STRING_HAS_MEMRCHR */
}

void *z_memtok (const void *s, const char *delim, unsigned int n) {
    register void *p;

    while (*delim != '\0') {
        if ((p = z_memchr(s, *delim++, n)) != NULL)
            return(p);
    }

    return(NULL);
}


