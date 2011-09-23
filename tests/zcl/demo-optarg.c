#include <zcl/optarg.h>

#include <stdio.h>

struct appopts {
    char *   op1;
    char **  op2;
    int      op3;
    int      op4;
    int32_t  op5;
    int64_t  op6;
    uint32_t op7;
    uint64_t op8;
};

static int __arg_my_func (int argc, char **argv, void *data) {
    printf("My Func %d\n", argc);
    return(0);
}

static int __arg_my_func2 (int argc, char **argv, void *data) {
    printf("My Func 2 %d %s %s\n", argc, argv[0], argv[1]);
    return(0);
}

static struct appopts __app_options = {
    .op1 = NULL,
    .op2 = NULL,
    .op3 = 0,
    .op4 = 0,
    .op5 = 0,
    .op6 = 0,
    .op7 = 0,
    .op8 = 0,
};

static z_optarg_t __options[] = {
    { "-h", "--help",    "Display this information.",
      z_optarg_show_help, 0, NULL },
    { "-1", "--op1",     "This is a Option 1 test.",
      z_optarg_store, 1, &(__app_options.op1) },
    { "-2", "--op2",     "This is a Option 2 test.",
      z_optarg_mstore, 3, &(__app_options.op2) },
    { "-3", "--op3",     "This is a Option 3 test.",
      z_optarg_store_true, 0, &(__app_options.op3) },
    { "-4", "--op4",     "This is a Option 4 test.",
      z_optarg_store_false, 0, &(__app_options.op4) },
    { "-5", "--op5",     "This is a Option 5 test.",
      z_optarg_store_int32, 1, &(__app_options.op5) },
    { "-6", "--op6",     "This is a Option 6 test.",
      z_optarg_store_int64, 1, &(__app_options.op6) },
    { "-7", "--op7",     "This is a Option 7 test.",
      z_optarg_store_uint32, 1, &(__app_options.op7) },
    { "-8", "--op8",     "This is a Option 8 test.",
      z_optarg_store_uint64, 1, &(__app_options.op8) },
    { "-f", "--my-func", "This is a custom function call test.",
      __arg_my_func, 0, &(__app_options) },
    { "-g", "--my-func2", "This is a custom function call test but with args.",
      __arg_my_func2, 2, &(__app_options) },
    { NULL, NULL, NULL, NULL, 0, NULL },
};



int main (int argc, char **argv) {
    if (z_optarg_parse(__options, argc, argv))
        return(1);
    return(0);
}


