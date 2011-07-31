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
#include <zcl/memcmp.h>
#include <zcl/memmem.h>

void *z_memmem (const void *haystack,
                unsigned int haystack_len,
                const void *needle,
                unsigned int needle_len)
{
#ifndef Z_STRING_HAS_MEMMEM
    register const uint8_t *pend;
    register const uint8_t *ph;
    register const uint8_t *pn;
    register uint8_t c;

    if (haystack_len < needle_len || needle_len == 0)
        return(NULL);

    pn = (const uint8_t *)needle;
    needle_len--;
    c = *pn++;

    ph = (const uint8_t *)haystack;
    pend = ph + haystack_len - needle_len;
    for (; ph <= pend; ++ph) {
        if (*ph == c && !z_memcmp(ph + 1, pn, needle_len))
            return((void *)ph);
    }

    return(NULL);
#else
    return(memmem(haystack, haystack_len, needle, needle_len));
#endif /* !Z_STRING_HAS_MEMMEM */
}

void *z_memrmem (const void *haystack,
                 unsigned int haystack_len,
                 const void *needle,
                 unsigned int needle_len)
{
#ifndef Z_STRING_HAS_MEMRMEM
    register const uint8_t *pend;
    register const uint8_t *ph;
    register const uint8_t *pn;
    register uint8_t c;

    if (haystack_len < needle_len || needle_len == 0)
        return(NULL);

    pn = (const uint8_t *)needle;
    needle_len--;
    c = *pn++;

    pend = (const uint8_t *)haystack;
    ph = pend + haystack_len - needle_len - 1;
    for (; ph >= pend; --ph) {
        if (*ph == c && !z_memcmp(ph + 1, pn, needle_len))
            return((void *)ph);
    }

    return(NULL);
#else
    return(memrmem(haystack, haystack_len, needle, needle_len));
#endif /* !Z_STRING_HAS_MEMRMEM */
}

void *z_memmem_horspool (const void *haystack,
                         unsigned int haystack_len,
                         const void *needle,
                         unsigned int needle_len)
{
    const uint8_t *ph = (const uint8_t *)haystack;
    const uint8_t *pn = (const uint8_t *)needle;
    unsigned int bcs[256];
    unsigned int i;

    for (i = 0; i < 256; ++i)
        bcs[i] = 1U + needle_len;

    for (i = 0; i < needle_len; ++i)
        bcs[pn[i]] = needle_len - i;

    while (haystack_len >= needle_len) {
        if (*ph == *pn && !z_memcmp(ph, pn, needle_len))
            return((void *)ph);

        i = bcs[ph[needle_len]];
        haystack_len -= i;
        ph += i;
    }

    return(NULL);
}

