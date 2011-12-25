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

#include <zcl/optarg.h>
#include <zcl/strtol.h>
#include <zcl/strcmp.h>
#include <zcl/strlen.h>

#include <stdio.h>

#define __optcmp(optname, arg)      (optname == NULL || z_strcmp(optname, arg))

static int __optarg_missing_args (z_optarg_t *opt, const char *name) {
    fprintf(stderr, "argument(s) to '%s' is missing.\n%s\n", name, opt->help);
    return(1);
}

int z_optarg_store (int argc, char **argv, void *data) {
    *((char **)data) = argv[0];
    return(0);
}

int z_optarg_mstore (int argc, char **argv, void *data) {
    *((char ***)data) = argv;
    return(0);
}

int z_optarg_store_true (int argc, char **argv, void *data) {
    *((int *)data) = 1;
    return(0);
}

int z_optarg_store_false (int argc, char **argv, void *data) {
    *((int *)data) = 0;
    return(0);
}

int z_optarg_store_int32 (int argc, char **argv, void *data) {
    return(!z_strtoi32(argv[0], 10, (int32_t *)data));
}

int z_optarg_store_int64 (int argc, char **argv, void *data) {
    return(!z_strtoi64(argv[0], 10, (int64_t *)data));
}

int z_optarg_store_uint32 (int argc, char **argv, void *data) {
    return(!z_strtou32(argv[0], 10, (uint32_t *)data));
}

int z_optarg_store_uint64 (int argc, char **argv, void *data) {
    return(!z_strtou64(argv[0], 10, (uint64_t *)data));
}

int z_optarg_show_help (int argc, char **argv, void *data) {
    unsigned int sname_length;
    unsigned int lname_length;
    unsigned int line_length;
    unsigned int n, i;
    const char *c;
    z_optarg_t *o;

    sname_length = 0;
    lname_length = 0;
    for (o = (z_optarg_t *)data; o->help != NULL; ++o) {
        i = (o->nargs > 0) ? ((o->nargs > 1) ? 11 : 4) : 0;

        if ((n = z_strlen(o->sname) + ((o->lname == NULL) ? i : 0)) > sname_length)
            sname_length = n;

        if ((n = z_strlen(o->lname) + i) > lname_length)
            lname_length = n;
    }

    line_length = 2 + sname_length + 1 + lname_length + 3;
    printf("Optional arguments:\n");
    for (o = (z_optarg_t *)data; o->help != NULL; ++o) {
        printf("  ");

        if (o->sname) {
            printf("%s ", o->sname);

            n = z_strlen(o->sname);
            if (o->lname == NULL) {
                if (o->nargs == 1) {
                    printf("ARG");
                    n += 3;
                } else if (o->nargs > 1) {
                    printf("ARG1..ARG%d", o->nargs);
                    n += 10;
                }
            }

            n = sname_length - n;
            while (n--) printf(" ");
        }

        if (o->lname) {
            printf("%s ", o->lname);

            n = z_strlen(o->lname);
            if (o->nargs == 1) {
                printf("ARG");
                n += 3;
            } else if (o->nargs > 1) {
                printf("ARG1..ARG%d", o->nargs);
                n += 10;
            }

            n = lname_length - n;
            while (n--) printf(" ");
        }

        printf("  ");
        c = o->help;
        n = z_strlen(o->help);
        while (n > 0) {
            i = (80 - line_length);
            while (i-- && n > 0) { printf("%c", *c++); n--; }
            printf("\n");

            if (n > 0) {
                i = line_length;
                while (i--) printf(" ");
                while (*c == ' ') c++;
            }
        }
    }

    return(1);
}

int z_optarg_parse (z_optarg_t *options, int argc, char **argv) {
    z_optarg_t *o;
    int i;

    for (i = 0; i < argc; ++i) {
        for (o = options; o->sname != NULL || o->lname != NULL; ++o) {
            if (__optcmp(o->sname, argv[i]) && __optcmp(o->lname, argv[i]))
                continue;

            /* Check for available option args */
            if (o->nargs >= (argc - i))
                return(__optarg_missing_args(o, argv[i]));

            if (o->func == z_optarg_show_help)
                return(z_optarg_show_help(argc, argv, (void *)options));

            if (o->func(o->nargs, argv + i + 1, o->data))
                return(1);

            i += o->nargs;
            break;
        }
    }

    return(0);
}

