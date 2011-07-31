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
#include <zcl/memcmp.h>

#define __cmp8_step(m1, m2)                                                 \
    do {                                                                    \
        if (m1[0] != m2[0]) return(m1[0] - m2[0]);                          \
    } while (0)

#define __cmp16_step(m1, m2)                                                \
    do {                                                                    \
        if (m1[0] != m2[0]) return(m1[0] - m2[0]);                          \
        if (m1[1] != m2[1]) return(m1[1] - m2[1]);                          \
    } while (0)

#define __cmp32_step(m1, m2)                                                \
    do {                                                                    \
        if (m1[0] != m2[0]) return(m1[0] - m2[0]);                          \
        if (m1[1] != m2[1]) return(m1[1] - m2[1]);                          \
        if (m1[2] != m2[2]) return(m1[2] - m2[2]);                          \
        if (m1[3] != m2[3]) return(m1[3] - m2[3]);                          \
    } while (0)

#define __cmp64_step(m1, m2)                                                \
    do {                                                                    \
        if (m1[0] != m2[0]) return(m1[0] - m2[0]);                          \
        if (m1[1] != m2[1]) return(m1[1] - m2[1]);                          \
        if (m1[2] != m2[2]) return(m1[2] - m2[2]);                          \
        if (m1[3] != m2[3]) return(m1[3] - m2[3]);                          \
        if (m1[4] != m2[4]) return(m1[4] - m2[4]);                          \
        if (m1[5] != m2[5]) return(m1[5] - m2[5]);                          \
        if (m1[6] != m2[6]) return(m1[6] - m2[6]);                          \
        if (m1[7] != m2[7]) return(m1[7] - m2[7]);                          \
    } while (0)

#define __cmp_generic_step(m1, m2, n)                                       \
    do {                                                                    \
        unsigned long i;                                                    \
        for (i = 0; i < n; ++i) {                                           \
            if (m1[i] != m2[i])                                             \
                return(m1[i] - m2[i]);                                      \
        }                                                                   \
    } while (0)

int z_memcmp (const void *m1, const void *m2, unsigned int n) {
#ifndef Z_STRING_HAS_MEMCMP
    const unsigned long *im1 = (const unsigned long *)m1;
    const unsigned long *im2 = (const unsigned long *)m2;
    const unsigned char *cm1;
    const unsigned char *cm2;

    /* Fast compare with n >= sizeof(unsigned long) */
    for (; n >= sizeof(unsigned long); n -= sizeof(unsigned long)) {
        if (*im1 != *im2) {
            cm1 = (const unsigned char *)im1;
            cm2 = (const unsigned char *)im2;

            switch (sizeof(unsigned long)) {
                case 1:
                    __cmp8_step(cm1, cm2);
                case 2:
                    __cmp16_step(cm1, cm2);
                    break;
                case 4:
                    __cmp32_step(cm1, cm2);
                    break;
                case 8:
                    __cmp64_step(cm1, cm2);
                    break;
                default:
                    __cmp_generic_step(cm1, cm2, sizeof(unsigned long));
                    break;
            }
        }

        im1++;
        im2++;
    }

    /* Compare the rest in a traditional way */
    cm1 = (const unsigned char *)im1;
    cm2 = (const unsigned char *)im2;
    for (; n--; cm1++, cm2++) {
        if (*cm1 != *cm2)
            return(*cm1 - *cm2);
    }

    return(0);
#else
    return(memcmp(m1, m2, n));
#endif /* !Z_STRING_HAS_MEMCMP */
}

int z_memcmp8 (const void *m1, const void *m2, unsigned int n) {
    const uint8_t *cm1 = (const uint8_t *)m1;
    const uint8_t *cm2 = (const uint8_t *)m2;

    for (; n--; cm1++, cm2++) {
        if (*cm1 != *cm2)
            return(*cm1 - *cm2);
    }

    return(0);
}

int z_memcmp16 (const void *m1, const void *m2, unsigned int n) {
    const uint16_t *im1 = (const uint16_t *)m1;
    const uint16_t *im2 = (const uint16_t *)m2;
    const uint8_t *cm1;
    const uint8_t *cm2;

    for (; n >= sizeof(uint16_t); n -= sizeof(uint16_t)) {
        if (*im1 != *im2) {
            cm1 = (const uint8_t *)im1;
            cm2 = (const uint8_t *)im2;
            __cmp16_step(cm1, cm2);
        }
        im1++;
        im2++;
    }

    return(0);
}

int z_memcmp32 (const void *m1, const void *m2, unsigned int n) {
    const uint32_t *im1 = (const uint32_t *)m1;
    const uint32_t *im2 = (const uint32_t *)m2;
    const uint8_t *cm1;
    const uint8_t *cm2;

    for (; n >= sizeof(uint32_t); n -= sizeof(uint32_t)) {
        if (*im1 != *im2) {
            cm1 = (const uint8_t *)im1;
            cm2 = (const uint8_t *)im2;
            __cmp32_step(cm1, cm2);
        }
        im1++;
        im2++;
    }

    return(0);
}

int z_memcmp64 (const void *m1, const void *m2, unsigned int n) {
    const uint64_t *im1 = (const uint64_t *)m1;
    const uint64_t *im2 = (const uint64_t *)m2;
    const uint8_t *cm1;
    const uint8_t *cm2;

    for (; n >= sizeof(uint64_t); n -= sizeof(uint64_t)) {
        if (*im1 != *im2) {
            cm1 = (const uint8_t *)im1;
            cm2 = (const uint8_t *)im2;
            __cmp64_step(cm1, cm2);
        }
        im1++;
        im2++;
    }

    return(0);
}
