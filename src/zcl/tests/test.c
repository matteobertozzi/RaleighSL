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

#include <zcl/test.h>

#include <stdio.h>

int z_test_run (const z_test_t *test, void *user_data) {
    unsigned int failure = 0;
    unsigned int total = 0;
    const z_test_func_t *f;

    for (f = test->funcs; *f != NULL; ++f, ++total) {
        if (test->setup != NULL && test->setup(user_data)) {
            fprintf(stderr, " [ !! ] %s failed setup.\n", test->name);
            return(-1);
        }

        if ((*f)(user_data))
            failure++;

        if (test->tear_down != NULL && test->tear_down(user_data)) {
            fprintf(stderr, " [ !! ] %s failed tear down.\n", test->name);
            return(-2);
        }
    }

    if (failure > 0) {
        printf(" [ !! ] %s succeded %u/%u (%.1f%% failed)\n", test->name,
               total - failure, total, (float)failure / total);
    } else {
        printf(" [ ok ] %s succeded %u/%u\n", test->name, total, total);
    }

    return(failure);
}