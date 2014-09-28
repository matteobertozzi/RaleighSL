/*
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

#include <zcl/search.h>

uint32_t z_upper_bound (void *base,
                        uint32_t end,
                        uint32_t stride,
                        const void *key,
                        z_compare_t key_cmp,
                        void *udata)
{
  uint32_t start = 0;
  while (start < end) {
    uint32_t mid = (start + end) >> 1;
    uint8_t *mitem = ((uint8_t *)base) + (mid * stride);
    int cmp = key_cmp(udata, mitem, key);
    if (cmp > 0) {
      end = mid;
    } else {
      start = mid + 1;
    }
  }
  return(start);
}