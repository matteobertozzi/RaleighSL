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

#include <stdlib.h>

#include <raleighfs/errno.h>

#define __ERR(x)                    RALEIGHFS_ERRNO_ ## x
#define __ERR_PLUGIN(x)             RALEIGHFS_ERRNO_PLUGIN_ ## x
#define __ERR_OBJECT(x)             RALEIGHFS_ERRNO_OBJECT_ ## x

const char *raleighfs_errno_string (raleighfs_errno_t errno) {
    errno &= ~RALEIGHFS_ERRNO_INFO;

    switch (errno) {
        case __ERR(NONE):               return("no error");
        case __ERR(NOT_IMPLEMENTED):    return("not implemented");

        /* System related */;
        case __ERR(NO_MEMORY):          return("no memory");

        /* Plugins related */
        case __ERR_PLUGIN(NOT_LOADED):  return("plugin not loaded");

        /* Semantic related */
        /* Object related */
        case __ERR_OBJECT(EXISTS):      return("object already exists");
        case __ERR_OBJECT(NOT_FOUND):   return("object not found");

        /* Device related */
        /* Format related */
        /* Space related */
        /* Key related */
    }

    return(NULL);
}

