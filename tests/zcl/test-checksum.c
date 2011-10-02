#include <stdio.h>

#include <zcl/checksum.h>
#include <zcl/test.h>

static int __test_crc16 (z_test_t *test) {
    if (z_csum16_crc(0, "a", 1) != 59585)
        return(1);

    if (z_csum16_crc(0, "ab", 2) != 31144)
        return(2);

    if (z_csum16_crc(0, "abc", 3) != 38712)
        return(3);

    return(0);
}

static int __test_adler32 (z_test_t *test) {
    if (z_csum32_adler(0, "a", 1) != 6357089U)
        return(1);

    if (z_csum32_adler(0, "ab", 2) != 19136707U)
        return(2);

    if (z_csum32_adler(0, "abc", 3) != 38404390U)
        return(3);

    return(0);
}

static int __test_crc32c (z_test_t *test) {
    if (z_csum32_crcc(0, "a", 1) != 2477592673U)
        return(1);

    if (z_csum32_crcc(0, "ab", 2) != 331570916U)
        return(2);

    if (z_csum32_crcc(0, "abc", 3) != 1445960909U)
        return(3);

    return(0);
}

static z_test_t __test_checksum = {
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_crc16,
        __test_adler32,
        __test_crc32c,
        NULL,
    },
};

int main (int argc, char **argv) {
    int res;

    if ((res = z_test_run(&__test_checksum, NULL)))
        printf(" [ !! ] Checksum %d\n", res);
    else
        printf(" [ ok ] Checksum\n");

    return(res);
}
