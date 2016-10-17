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

#ifndef _Z_SORT_H_
#define _Z_SORT_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

void z_quick3_str     (uint8_t *array[], int lo, int hi);
void z_quick3_u32     (uint32_t array[], int lo, int hi);
void z_quick3_u64     (uint64_t array[], int lo, int hi);

void z_heap_sort      (void *base, size_t num, size_t size,
                       int (*cmp_func) (void *, void *, void *),
                       void *udata);
void z_heap_sort_u32  (uint32_t array[], int length);
void z_heap_sort_u64  (uint64_t array[], int length);
void z_heap_sort_item (void *udata, int n,
                       int (*cmp_func) (void *, int, int),
                       void (*swap_func) (void *, int, int));

__Z_END_DECLS__

#endif /* !_Z_SORT_H_ */
