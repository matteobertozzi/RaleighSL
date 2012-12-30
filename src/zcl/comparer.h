/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

#ifndef _Z_COMPARER_H_
#define _Z_COMPARER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

typedef int (*z_compare_t) (void *obj, const void *a, const void *b);

struct z_vtable_comparer {
    int     (*equals)           (const void *self, const void *other);
    int     (*compare)          (const void *self, const void *other);
};

__Z_END_DECLS__

#endif /* !_Z_COMPARER_H_ */

