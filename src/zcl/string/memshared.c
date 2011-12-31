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

unsigned int memshared (const void *a,
                        unsigned int alen,
                        const void *b,
                        unsigned int blen)
{
    const unsigned long *ia = (const unsigned long *)a;
    const unsigned long *ib = (const unsigned long *)b;
    const unsigned char *ca;
    const unsigned char *cb;
    unsigned int n, min_length;

    n = min_length = (alen < blen) ? alen : blen;
    while (n >= sizeof(unsigned long)) {
        if (*ia != *ib)
            break;

        n -= sizeof(unsigned long);
        ia++;
        ib++;
    }

    ca = (const unsigned char *)ia;
    cb = (const unsigned char *)ib;
    while (n > 0 && *ca++ == *cb++)
        n--;

    return(min_length - n);
}

unsigned int memrshared (const void *a,
                         unsigned int alen,
                         const void *b,
                         unsigned int blen)
{
    const unsigned long *ia = (const unsigned long *)(((char *)a) + alen);
    const unsigned long *ib = (const unsigned long *)(((char *)b) + blen);
    const unsigned char *ca;
    const unsigned char *cb;
    unsigned int n, min_length;

    n = min_length = (alen < blen) ? alen : blen;
    while (n >= sizeof(unsigned long)) {
        if (*(ia - 1) != *(ib - 1))
            break;

        n -= sizeof(unsigned long);
        ia--;
        ib--;
    }

    ca = (const unsigned char *)ia;
    cb = (const unsigned char *)ib;
    while (n > 0 && *--ca == *--cb)
        n--;

    return(min_length - n);
}

