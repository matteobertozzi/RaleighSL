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

#ifndef _Z_OPTARG_H_
#define _Z_OPTARG_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_CONST_STRUCT(z_optarg)

#define Z_OPTFUNC_DECLARE(name)                                             \
    int name (int argc, char **argv, void *data);

typedef int (*z_optfunc_t) (int argc, char **argv, void *data);

struct z_optarg {
    const char * sname;             /* Short name (e.g. -h) */
    const char * lname;             /* Long name (e.g. --help) */
    const char * help;              /* Help message */
    z_optfunc_t  func;              /* Option function */
    unsigned int nargs;             /* Number of arguments */
    void *       data;              /* Option data passed to function */
};

Z_OPTFUNC_DECLARE(z_optarg_store)
Z_OPTFUNC_DECLARE(z_optarg_mstore)
Z_OPTFUNC_DECLARE(z_optarg_store_true)
Z_OPTFUNC_DECLARE(z_optarg_store_false)
Z_OPTFUNC_DECLARE(z_optarg_store_int32)
Z_OPTFUNC_DECLARE(z_optarg_store_int64)
Z_OPTFUNC_DECLARE(z_optarg_store_uint32)
Z_OPTFUNC_DECLARE(z_optarg_store_uint64)
Z_OPTFUNC_DECLARE(z_optarg_show_help)

int z_optarg_parse (z_optarg_t *options, int argc, char **argv);

__Z_END_DECLS__

#endif /* !_Z_OPTARG_H_ */

