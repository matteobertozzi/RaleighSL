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

#ifndef _Z_TEST_H_
#define _Z_TEST_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#ifndef Z_TEST_MAX
    #define Z_TEST_MAX          255
#endif /* !Z_TEST_MAX */

Z_TYPEDEF_STRUCT(z_test)

typedef int (*z_test_func_t) (z_test_t *test);

struct z_test {
    int             (*setup)        (z_test_t *test);
    int             (*tear_down)    (z_test_t *test);

    z_test_func_t   funcs[Z_TEST_MAX];
    void *          user_data;
};

int     z_test_run      (z_test_t *test, void *user_data);

__Z_END_DECLS__

#endif /* !_Z_TEST_H_ */

