/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#ifndef _Z_TEST_H_
#define _Z_TEST_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <stdio.h>

#ifndef Z_TEST_MAX
    #define Z_TEST_MAX          255
#endif /* !Z_TEST_MAX */

Z_TYPEDEF_STRUCT(z_test)

typedef int (*z_test_func_t) (void *self);

struct z_test {
    const char *name;

    int             (*setup)        (void *self);
    int             (*tear_down)    (void *self);

    z_test_func_t   funcs[Z_TEST_MAX];
};

int     z_test_run      (const z_test_t *test, void *user_data);

#define z_test_equals(expected, value)                                      \
    z_test_assert((long)(expected) != (long)(value), "expected %ld is %ld", \
                  (long)expected, (long)value);

#define z_test_true(value)                                                  \
    z_test_assert(!(value), "expected true is %ld", (long)value);

#define z_test_false(value)                                                 \
    z_test_assert(!!(value), "expected false is %ld", (long)value);

#define z_test_assert(cond, format, ...)                                    \
    if (cond) {                                                             \
      fprintf(stderr, "%s:%d %s - " format "\n",                            \
              __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);             \
      return(1);                                                            \
    }

__Z_END_DECLS__

#endif /* !_Z_TEST_H_ */