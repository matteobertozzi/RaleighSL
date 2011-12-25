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
#include <zcl/memcpy.h>

#define __memcpy_forward(dst, src, n)                                       \
    do {                                                                    \
        const unsigned long *isrc = (const unsigned long *)src;             \
        unsigned long *idst = (unsigned long *)dst;                         \
        const unsigned char *csrc;                                          \
        unsigned char *cdst;                                                \
                                                                            \
        /* Fast copy while l >= sizeof(unsigned long) */                    \
        for (; n >= sizeof(unsigned long); n -= sizeof(unsigned long))      \
            *idst++ = *isrc++;                                              \
                                                                            \
        /* Copy the rest of the buffer */                                   \
        csrc = (const unsigned char *)isrc;                                 \
        cdst = (unsigned char *)idst;                                       \
        while (n--)                                                         \
            *cdst++ = *csrc++;                                              \
    } while (0)

#define __memcpy_forward_sized(type, dst, src, n)                           \
    do {                                                                    \
        const type *isrc = (const type *)src;                               \
        type *idst = (type *)dest;                                          \
                                                                            \
        for (; n >= sizeof(type); n -= sizeof(type))                        \
            *idst++ = *isrc++;                                              \
    } while (0)

#define __memcpy_backward(dst, src, n)                                      \
    do {                                                                    \
        const unsigned char *csrc = (const unsigned char *)src;             \
        unsigned char *cdst = (unsigned char *)dst;                         \
        const unsigned long *isrc;                                          \
        unsigned long *idst;                                                \
                                                                            \
        idst = (unsigned long *)(cdst + n);                                 \
        isrc = (const unsigned long *)(csrc + n);                           \
        for (; n >= sizeof(unsigned long); n -= sizeof(unsigned long))      \
            *(--idst) = *(--isrc);                                          \
                                                                            \
        csrc = (const unsigned char *)isrc;                                 \
        cdst = (unsigned char *)idst;                                       \
        while (n--)                                                         \
            *(--cdst) = *(--csrc);                                          \
    } while (0)

#define __memcpy_backward_sized(type, dst, src, n)                          \
    do {                                                                    \
        const type *isrc = (const type *)(((const char *)src) + n);         \
        type *idst = (type *)(((const char *)dst) + n);                     \
                                                                            \
        for (; n > sizeof(type); n -= sizeof(type))                         \
            *(--idst) = *(--isrc);                                          \
    } while (0)

/*
 * Mem-Copy Forward
 */
#ifndef Z_STRING_HAS_MEMCPY
void *z_memcpy (void *dest, const void *src, unsigned int n) {
    __memcpy_forward(dest, src, n);
    return(dest);
}
#endif /* !Z_STRING_HAS_MEMCPY */

void *z_memcpy8 (void *dest, const void *src, unsigned int n) {
    __memcpy_forward_sized(uint8_t, dest, src, n);
    return(dest);
}

void *z_memcpy16 (void *dest, const void *src, unsigned int n) {
    __memcpy_forward_sized(uint16_t, dest, src, n);
    return(dest);
}

void *z_memcpy32 (void *dest, const void *src, unsigned int n) {
    __memcpy_forward_sized(uint32_t, dest, src, n);
    return(dest);
}

void *z_memcpy64 (void *dest, const void *src, unsigned int n) {
    __memcpy_forward_sized(uint64_t, dest, src, n);
    return(dest);
}

/*
 * Mem-Copy backward
 */
void *z_membcpy (void *dest, const void *src, unsigned int n) {
    __memcpy_backward(dest, src, n);
    return(dest);
}

void *z_membcpy8 (void *dest, const void *src, unsigned int n) {
    __memcpy_backward_sized(uint8_t, dest, src, n);
    return(dest);
}

void *z_membcpy16 (void *dest, const void *src, unsigned int n) {
    __memcpy_backward_sized(uint16_t, dest, src, n);
    return(dest);
}

void *z_membcpy32 (void *dest, const void *src, unsigned int n) {
    __memcpy_backward_sized(uint32_t, dest, src, n);
    return(dest);
}

void *z_membcpy64 (void *dest, const void *src, unsigned int n) {
    __memcpy_backward_sized(uint64_t, dest, src, n);
    return(dest);
}

/*
 * Mem-Move
 */
#ifndef Z_STRING_HAS_MEMMOVE
void *z_memmove (void *dest, const void *src, unsigned int n) {
    if (dest <= src || dest >= (src + n))
        __memcpy_forward(dest, src, n);
    else
        __memcpy_backward(dest, src, n);
    return(dest);
}
#endif /* !Z_STRING_HAS_MEMMOVE */

void *z_memmove8 (void *dest, const void *src, unsigned int n) {
    if (dest <= src || (const char *)dest >= ((const char *)src + n))
        __memcpy_forward_sized(uint8_t, dest, src, n);
    else
        __memcpy_backward_sized(uint8_t, dest, src, n);
    return(dest);
}

void *z_memmove16 (void *dest, const void *src, unsigned int n) {
    if (dest <= src || (const char *)dest >= ((const char *)src + n))
        __memcpy_forward_sized(uint16_t, dest, src, n);
    else
        __memcpy_backward_sized(uint16_t, dest, src, n);
    return(dest);
}

void *z_memmove32 (void *dest, const void *src, unsigned int n) {
    if (dest <= src || (const char *)dest >= ((const char *)src + n))
        __memcpy_forward_sized(uint32_t, dest, src, n);
    else
        __memcpy_backward_sized(uint32_t, dest, src, n);
    return(dest);
}

void *z_memmove64 (void *dest, const void *src, unsigned int n) {
    if (dest <= src || (const char *)dest >= ((const char *)src + n))
        __memcpy_forward_sized(uint64_t, dest, src, n);
    else
        __memcpy_backward_sized(uint64_t, dest, src, n);
    return(dest);
}

