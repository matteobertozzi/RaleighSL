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

#ifndef _Z_BSEARCH_H_
#define _Z_BSEARCH_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/types.h>

void *      z_bsearch           (const void *key,
                                 const void *base,
                                 unsigned int num,
                                 unsigned int size,
                                 z_compare_t cmp_func,
                                 void *user_data);

void *      z_bsearch_index     (const void *key,
                                 const void *base,
                                 unsigned int num,
                                 unsigned int size,
                                 z_compare_t cmp_func,
                                 void *user_data,
                                 unsigned int *index);

__Z_END_DECLS__

#endif /* !_Z_BSEARCH_H_ */
