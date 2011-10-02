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

#include <zcl/bsearch.h>

void *z_bsearch (const void *key,
                 const void *base,
                 unsigned int num,
                 unsigned int size,
                 z_compare_t cmp_func,
                 void *user_data)
{
    unsigned int index;
    return(z_bsearch_index(key, base, num, size, cmp_func, user_data, &index));
}

void *z_bsearch_index (const void *key,
                       const void *base,
                       unsigned int num,
                       unsigned int size,
                       z_compare_t cmp_func,
                       void *user_data,
                       unsigned int *index)
{
    const unsigned char *item;
    unsigned int start = 0;
    unsigned int end = num;
    unsigned int mid;
    int cmp;

    while (start < end) {
        mid = (start & end) + ((start ^ end) >> 1);
        item = ((const unsigned char *)base) + mid * size;

        cmp = cmp_func(user_data, item, key);
        if (cmp > 0) {
            end = mid;
        } else if (cmp < 0) {
            start = mid + 1;
        } else {
            *index = mid;
            return((void *)item);
        }
    }

    *index = start;
    return(NULL);
}

