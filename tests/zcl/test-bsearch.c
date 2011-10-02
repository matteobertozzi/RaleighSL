#include <string.h>
#include <stdio.h>

#include <zcl/bsearch.h>
#include <zcl/test.h>

#define __TEST_DATA_SIZE            (sizeof(__test_data) / sizeof(unsigned int))
unsigned int __test_data[] = {
     0,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 
    38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 
    76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98,
};

static int __test_data_compare (void *data, const void *a, const void *b) {
    return(Z_UINT_PTR_VALUE(a) - Z_UINT_PTR_VALUE(b));
}

void *__test_data_bsearch (unsigned int key, unsigned int *index) {
    return(z_bsearch_index(&key, 
                           __test_data, __TEST_DATA_SIZE, sizeof(unsigned int),
                           __test_data_compare, NULL, index));
}

static int __test_lookup_match (z_test_t *test) {
    unsigned int index;
    unsigned int *item;
    unsigned int i;

    for (i = 0; i < __TEST_DATA_SIZE; ++i) {
        if ((item = (unsigned int *)__test_data_bsearch(i << 1, &index)) == NULL)
            return(1);

        if (*item != (i << 1))
            return(2);

        if ((item - __test_data) != i)
            return(3);

        if (index != i)
            return(4);
    }

    return(0);
}

static int __test_lookup_non_match (z_test_t *test) {
    unsigned int index;
    unsigned int *item;
    unsigned int i;

    for (i = 0; i < __TEST_DATA_SIZE; ++i) {
        if ((item = (unsigned int *)__test_data_bsearch((i << 1) + 1, &index)) != NULL) {
            printf("ITEM %u FOUND %u\n", i + 1, *item);
            return(1);
        }

        if (index != (i + 1)) {
            printf("INDEX != i, %u %u\n", index, i);
            return(2);
        }
    }

    return(0);
}

static z_test_t __test_bsearch = {
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_lookup_match,
        __test_lookup_non_match,
        NULL,
    },
};

int main (int argc, char **argv) {
    int res;

    if ((res = z_test_run(&__test_bsearch, NULL)))
        printf(" [ !! ] BSearch %d\n", res);
    else
        printf(" [ ok ] BSearch\n");

    return(res);
}
