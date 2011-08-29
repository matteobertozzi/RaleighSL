#include <string.h>
#include <stdio.h>

#include <zcl/snprintf.h>
#include <zcl/string.h>
#include <zcl/memcmp.h>
#include <zcl/test.h>

struct __strtol_data {
    const char *str;
    int base;
    int valid;
    int num;
} __test_strtol_data[] = {
    { "  0xK2  ",      16, 0, 0  },
    { " \t0x16ZOR\n",  16, 1, 22 },
    { "  \n0xABC",     16, 1, 2748 },
    { "ABC28",         16, 1, 703528 },
    { " \n0b1111\t",    2, 1, 15 },
    { " \n0b5124\n",    2, 0, 0 },
    { "0200",           8, 1, 128 },
    { "10",             8, 1, 8 },
    { "-23",           10, 1, -23 },
    { "-*7",           10, 0, 0 },
    { "   -52L",       10, 1, -52 },
    { NULL, 0, 0 },
};

static int __test_strtol (z_test_t *test) {
    const struct __strtol_data *p;
    uint64_t u64;
    uint32_t u32;
    int64_t i64;
    int32_t i32;

    for (p = __test_strtol_data; p->str != NULL; ++p) {
        if (p->num >= 0) {
            if (z_strtou32(p->str, p->base, &u32) != p->valid)
                return(1);

            if (p->valid && u32 != p->num)
                return(2);

            if (z_strtou64(p->str, p->base, &u64) != p->valid)
                return(3);

            if (p->valid && u64 != p->num)
                return(4);
        }

        if (z_strtoi32(p->str, p->base, &i32) != p->valid)
            return(5);

        if (p->valid && i32 != p->num)
            return(6);

        if (z_strtoi64(p->str, p->base, &i64) != p->valid)
            return(7);

        if (p->valid && i64 != p->num)
            return(8);
    }


    return(0);
}

static int __test_snprintf (z_test_t *test) {
    char buffer[128];
    uint64_t u64 = 64;
    uint32_t u32 = 32;
    uint16_t u16 = 16;
    int64_t i64 = -64;
    int32_t i32 = -32;
    int16_t i16 = -16;
    uint8_t u8 = 8;
    int8_t i8 = -8;
    int n;

    n = z_snprintf(buffer, sizeof(buffer),
                   "%u1d %u2d %u4d %u8d - %i1d %i2d %i4d %i8d",
                   u8, u16, u32, u64, i8, i16, i32, i64);
    if (n != 27 || z_memcmp(buffer, "8 16 32 64 - -8 -16 -32 -64", 27))
        return(1);

    n = z_snprintf(buffer, sizeof(buffer),
                   "%u1b %u2b %u4b %u8b - %i1b %i2b %i4b %i8b",
                   u8, u16, u32, u64, i8, i16, i32, i64);
    if (n != 57 || z_memcmp(buffer, "1000 10000 100000 1000000 - -1000 -10000 -100000 -1000000", 57))
        return(2);

    n = z_snprintf(buffer, sizeof(buffer),
                   "%u1o %u2o %u4o %u8o - %i1o %i2o %i4o %i8o",
                   u8, u16, u32, u64, i8, i16, i32, i64);
    if (n != 31 || z_memcmp(buffer, "10 20 40 100 - -10 -20 -40 -100", 31))
        return(3);

    n = z_snprintf(buffer, sizeof(buffer),
                   "%u1x %u2x %u4x %u8x - %i1x %i2x %i4x %i8x",
                   u8, u16, u32, u64, i8, i16, i32, i64);
    if (n != 27 || z_memcmp(buffer, "8 10 20 40 - -8 -10 -20 -40", 27))
        return(4);

    n = z_snprintf(buffer, 17,
                   "%u1d %u2d %u4d %u8d - %i1d %i2d %i4d %i8d",
                   u8, u16, u32, u64, i8, i16, i32, i64);
    if (n != 16 || z_memcmp(buffer, "8 16 32 64 - -8 ", 16))
        return(5);

    n = z_snprintf(buffer, sizeof(buffer), "Just a format test %% %c %s %s %u4d\n",
                   'K', NULL, "Hello String", 0xABC);
    if (n != 48 || z_memcmp(buffer, "Just a format test % K (none) Hello String 2748\n", 48))
        return(6);

    return(0);
}

static z_test_t __test_string = {
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_strtol,
        __test_snprintf,
        NULL,
    },
};

int main (int argc, char **argv) {
    int res;

    if ((res = z_test_run(&__test_string, NULL)))
        printf(" [ !! ] String %d\n", res);
    else
        printf(" [ ok ] String\n");

    return(res);
}
