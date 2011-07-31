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

#include <ctype.h>

#include <zcl/strupper.h>

#define __toupper(c)     ((!isalpha(c) || isupper(c)) ? (c) : ((c) - 'a' + 'A'))

char *z_strupper (char *str) {
    char *p;

    for (p = str; *p != '\0'; ++p)
        *p = __toupper(*p);

    return(str);
}

char *z_strnupper (char *str, unsigned int n) {
    char *p;

    for (p = str; n--; ++p)
        *p = __toupper(*p);

    return(str);
}

