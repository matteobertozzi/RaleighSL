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

#ifndef _Z_STRING_H_
#define _Z_STRING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#include <zcl/memchr.h>
#include <zcl/memcpy.h>
#include <zcl/memcmp.h>
#include <zcl/memmem.h>
#include <zcl/memset.h>
#include <zcl/memswap.h>

#include <zcl/strlen.h>
#include <zcl/strcmp.h>
#include <zcl/strcpy.h>
#include <zcl/strstr.h>
#include <zcl/strtol.h>
#include <zcl/strtrim.h>
#include <zcl/strlower.h>
#include <zcl/strupper.h>

Z_TYPEDEF_STRUCT(z_string)

struct z_string {
    const char  *blob;
    unsigned int length;
};

#define z_string_set(str, s, n)                                             \
    do {                                                                    \
        (str)->blob = (s);                                                  \
        (str)->length = (n);                                                \
    } while (0)

__Z_END_DECLS__

#endif /* !_Z_STRING_H_ */
