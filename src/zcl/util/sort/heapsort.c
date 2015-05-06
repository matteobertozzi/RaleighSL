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

#include <zcl/memutil.h>
#include <zcl/sort.h>

void z_heap_sort(void *base, size_t num, size_t size,
                 z_compare_t cmp_func, void *udata)
{
  int i = (num/2 - 1) * size;
  int n = num * size;

  for (; i >= 0; i -= size) {
    int c = i * 2 + size;
    int r = i;
    while (c < n) {
      if (c < n - size && cmp_func(udata, base + c, base + c + size) < 0)
        c += size;
      if (cmp_func(udata, base + r, base + c) >= 0)
        break;
      z_memswap(base + r, base + c, size);

      r = c;
      c = r * 2 + size;
    }
  }

  for (i = n - size; i > 0; i -= size) {
    int c = size;
    int r = 0;
    z_memswap(base, base + i, size);
    while (c < i) {
      if (c < i - size && cmp_func(udata, base + c, base + c + size) < 0)
        c += size;
      if (cmp_func(udata, base + r, base + c) >= 0)
        break;
      z_memswap(base + r, base + c, size);

      r = c;
      c = r * 2 + size;
    }
  }
}

void z_heap_sort_u32 (uint32_t array[], int n) {
  int i = (n / 2 - 1);

  for (; i >= 0; --i) {
    int c = i * 2 + 1;
    int r = i;
    while (c < n) {
      if (c < n - 1 && array[c] < array[c+1])
        c += 1;
      if (array[r] >= array[c])
        break;
      z_type_swap(uint32_t, array[r], array[c]);

      r = c;
      c = r * 2 + 1;
    }
  }

  for (i = n - 1; i > 0; --i) {
    int c = 1;
    int r = 0;
    z_type_swap(uint32_t, array[0], array[i]);
    while (c < i) {
      if (c < i - 1 && array[c] < array[c + 1])
        c += 1;
      if (array[r] >= array[c])
        break;
      z_type_swap(uint32_t, array[r], array[c]);

      r = c;
      c = r * 2 + 1;
    }
  }
}

void z_heap_sort_u64 (uint64_t array[], int n) {
  int i = (n / 2 - 1);

  for (; i >= 0; --i) {
    int c = i * 2 + 1;
    int r = i;
    while (c < n) {
      if (c < n - 1 && array[c] < array[c+1])
        c += 1;
      if (array[r] >= array[c])
        break;
      z_type_swap(uint64_t, array[r], array[c]);

      r = c;
      c = r * 2 + 1;
    }
  }

  for (i = n - 1; i > 0; --i) {
    int c = 1;
    int r = 0;
    z_type_swap(uint64_t, array[0], array[i]);
    while (c < i) {
      if (c < i - 1 && array[c] < array[c + 1])
        c += 1;
      if (array[r] >= array[c])
        break;
      z_type_swap(uint64_t, array[r], array[c]);

      r = c;
      c = r * 2 + 1;
    }
  }
}

void z_heap_sort_item (void *udata, int n,
                       int (*cmp_func) (void *, int, int),
                       void (*swap_func) (void *, int, int))
{
  int i = (n / 2 - 1);

  for (; i >= 0; --i) {
    int c = i * 2 + 1;
    int r = i;
    while (c < n) {
      if (c < n - 1 && cmp_func(udata, c, c + 1) < 0)
        c += 1;
      if (cmp_func(udata, r, c) >= 0)
        break;
      swap_func(udata, r, c);

      r = c;
      c = r * 2 + 1;
    }
  }

  for (i = n - 1; i > 0; --i) {
    int c = 1;
    int r = 0;
    swap_func(udata, 0, i);
    while (c < i) {
      if (c < i - 1 && cmp_func(udata, c, c + 1) < 0)
        c += 1;
      if (cmp_func(udata, r, c) >= 0)
        break;
      swap_func(udata, r, c);

      r = c;
      c = r * 2 + 1;
    }
  }
}
