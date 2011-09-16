#include <string.h>
#include <stdio.h>

#include <zcl/bitops.h>
#include <zcl/string.h>
#include <zcl/memcmp.h>
#include <zcl/test.h>

static int __test_ff (z_test_t *test) {
    if (z_ffz64(0xffffffffffffffff) != 0)
        return(1);

    if (z_ffs64(0xffffffffffffffff) != 1)
        return(2);

    if (z_ffs32(0x443) != 1)
        return(3);

    if (z_ffz32(0x443) != 3)
        return(4);

    return(0);
}

static int __test_ff_blob (z_test_t *test) {
    uint8_t bits[] = "01010101000000001010101011111111010101010000000010101010";
    uint8_t map[] = {0xAA, 0x00, 0x55, 0xFF, 0xAA, 0x00, 0x55};
    unsigned int size = sizeof(map) << 3;
    unsigned int i, x;

    for (i = 0; i < size - 1; ++i) {
        x = z_ffs(map, i, size);
        if (bits[x - 1] != '1')
            return(1);
    }

    for (i = 0; i < size; ++i) {
        x = z_ffz(map, i, size);
        if (bits[x - 1] != '0')
            return(2);
    }

    return(0);
}

static int __test_ff_area (z_test_t *test) {
    /* 0101010100000000101010101111111101010101000000001010101000000000000000001111111111111111 */
    uint8_t map[] = {
        0xAA, 0x00, 0x55, 0xFF, 0xAA, 0x00, 0x55, 0x00, 0x00, 0xFF, 0xFF
    };
    unsigned int size = sizeof(map) << 3;

    if (z_ffs_area(map, 0, size, 1) != 2)  return(1);
    if (z_ffs_area(map, 0, size, 2) != 25) return(2);
    if (z_ffs_area(map, 0, size, 3) != 25) return(3);
    if (z_ffs_area(map, 0, size, 4) != 25) return(4);
    if (z_ffs_area(map, 0, size, 5) != 25) return(5);
    if (z_ffs_area(map, 0, size, 6) != 25) return(6);
    if (z_ffs_area(map, 0, size, 7) != 25) return(7);
    if (z_ffs_area(map, 0, size, 8) != 25) return(8);
    if (z_ffs_area(map, 0, size, 9) != 73) return(9);
    if (z_ffs_area(map, 0, size, 10) != 73) return(10);
    if (z_ffs_area(map, 0, size, 12) != 73) return(11);
    if (z_ffs_area(map, 0, size, 16) != 73) return(12);
    if (z_ffs_area(map, 0, size, 17) != 0) return(13);
    if (z_ffs_area(map, 24, size, 8) != 25) return(14);
    if (z_ffs_area(map, 27, size, 8) != 73) return(15);

    if (z_ffz_area(map, 0, size, 1) != 1) return(16);
    if (z_ffz_area(map, 0, size, 2) != 9) return(17);
    if (z_ffz_area(map, 0, size, 3) != 9) return(18);
    if (z_ffz_area(map, 0, size, 4) != 9) return(19);
    if (z_ffz_area(map, 0, size, 7) != 9) return(20);
    if (z_ffz_area(map, 0, size, 8) != 9) return(21);
    if (z_ffz_area(map, 0, size, 9) != 56) return(22);
    if (z_ffz_area(map, 0, size, 10) != 56) return(23);
printf("STEP 1\n");
    if (z_ffz_area(map, 0, size, 12) != 56) return(24);
printf("STEP 2\n");
    if (z_ffz_area(map, 0, size, 16) != 56) return(25);
printf("STEP 3\n");
    if (z_ffz_area(map, 0, size, 17) != 56) return(26);
    if (z_ffz_area(map, 0, size, 18) != 0) return(27);
    printf("%d\n", (z_ffz_area(map, 11, size, 7)));
    if (z_ffz_area(map, 11, size, 4) != 12) return(28);
    if (z_ffz_area(map, 11, size, 7) != 41) return(29);

    return(0);
}

static int __test_bit_test (z_test_t *test) {
    unsigned int i;
    uint32_t x;

    /* Check for all 0 */
    x = 0x00000000;
    for (i = 0; i < 32; ++i) {
        if (z_bit_test(&x, i))
            return(1);
    }

    /* Check for all 1 */
    x = 0xffffffff;
    for (i = 0; i < 32; ++i) {
        if (!z_bit_test(&x, i))
            return(2);
    }

    /* Check for all 01 */
    x = 0xaaaaaaaa;
    for (i = 0; i < 32; ++i) {
        if (z_bit_test(&x, i) != (i & 1))
            return(3);
    }

    /* Check for all 10 */
    x = 0x55555555;
    for (i = 0; i < 32; ++i) {
        if (z_bit_test(&x, i) == (i & 1))
            return(4);
    }

    return(0);
}

static int __test_bit_set (z_test_t *test) {
    unsigned int i, j;
    uint32_t x;

    x = 0;
    for (i = 0; i < 32; ++i) {
        z_bit_set(&x, i);
        for (j = 0; j < 32; ++j) {
            if (z_bit_test(&x, j) != (j <= i))
                return(1);
        }
    }

    if (x != 0xffffffff)
        return(2);

    return(0);
}

static int __test_bit_clear (z_test_t *test) {
    unsigned int i, j;
    uint32_t x;

    x = 0xffffffff;
    for (i = 0; i < 32; ++i) {
        z_bit_clear(&x, i);
        for (j = 0; j < 32; ++j) {
            if (z_bit_test(&x, j) == (j <= i))
                return(1);
        }
    }

    if (x != 0)
        return(2);

    return(0);
}

static int __test_bit_toggle (z_test_t *test) {
    unsigned int i;
    uint32_t x;

    x = 0xaaaaaaaa;
    for (i = 0; i < 32; ++i) z_bit_toggle(&x, i);
    if (x != 0x55555555) return(1);

    for (i = 0; i < 32; ++i) z_bit_toggle(&x, i);
    if (x != 0xaaaaaaaa) return(2);

    return(0);
}

static int __test_bit_change (z_test_t *test) {
    unsigned int i;
    uint32_t x;

    x = 0;
    for (i = 0; i < 32; ++i) z_bit_change(&x, i, i & 1);
    if (x != 0xaaaaaaaa) return(1);


    for (i = 0; i < 32; ++i) z_bit_change(&x, i, !(i & 1));
    if (x != 0x55555555) return(2);

    return(0);
}

static z_test_t __test_bitopts = {
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_ff,
        __test_ff_blob,
        __test_ff_area,
        __test_bit_test,
        __test_bit_set,
        __test_bit_clear,
        __test_bit_toggle,
        __test_bit_change,
        NULL,
    },
};

int main (int argc, char **argv) {
    int res;

    if ((res = z_test_run(&__test_bitopts, NULL)))
        printf(" [ !! ] BitOpts %d\n", res);
    else
        printf(" [ ok ] BitOpts\n");

    return(res);
}

