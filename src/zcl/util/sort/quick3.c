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

#include <zcl/sort.h>

static void __quick3_str (uint8_t *a[], int lo, int hi, int d) {
  int i, v, lt, gt;

  lt = lo;
  gt = hi;
  v = a[lo][d];
  i = lo + 1;
  while (i <= gt) {
    const int t = a[i][d];
    if (t < v) {
      uint8_t *x = a[lt];
      a[lt++] = a[i];
      a[i++]  = x;
    } else if (t > v) {
      uint8_t *x = a[i];
      a[i] = a[gt];
      a[gt--] = x;
    } else {
      i++;
    }
  }

  if ((lt - 1) > lo)
    __quick3_str(a, lo, lt - 1, d);

  if (v >= 0 && gt > lt)
    __quick3_str(a, lt, gt, d + 1);

  if (hi > (gt + 1))
    __quick3_str(a, gt + 1, hi, d);
}

void z_quick3_str (uint8_t *array[], int lo, int hi) {
  __quick3_str(array, lo, hi - 1, 0);
}

void z_quick3_u32 (uint32_t a[], int lo, int hi) {
  int i, lt, gt;
  uint32_t v;

  lt = lo;
  gt = hi;
  v = a[lo];
  i = lo + 1;
  while (i <= gt) {
    const uint32_t t = a[i];
    if (t < v) {
      uint32_t x = a[lt];
      a[lt++] = a[i];
      a[i++]  = x;
    } else if (t > v) {
      uint32_t x = a[i];
      a[i] = a[gt];
      a[gt--] = x;
    } else {
      i++;
    }
  }

  if ((lt - 1) > lo)
    z_quick3_u32(a, lo, lt - 1);

  if (v > 0 && gt > lt)
    z_quick3_u32(a, lt, gt);

  if (hi > (gt + 1))
    z_quick3_u32(a, gt + 1, hi);
}

void z_quick3_u64 (uint64_t a[], int lo, int hi) {
  int i, lt, gt;
  uint64_t v;

  lt = lo;
  gt = hi;
  v = a[lo];
  i = lo + 1;
  while (i <= gt) {
    const uint64_t t = a[i];
    if (t < v) {
      uint64_t x = a[lt];
      a[lt++] = a[i];
      a[i++]  = x;
    } else if (t > v) {
      uint64_t x = a[i];
      a[i] = a[gt];
      a[gt--] = x;
    } else {
      i++;
    }
  }

  if ((lt - 1) > lo)
    z_quick3_u64(a, lo, lt - 1);

  if (v > 0 && gt > lt)
    z_quick3_u64(a, lt, gt);

  if (hi > (gt + 1))
    z_quick3_u64(a, gt + 1, hi);
}
