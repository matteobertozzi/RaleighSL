#include <stdio.h>

int main (int argc, char **argv) {
    int x = 0;
    int r;

    r = __sync_fetch_and_add(&x, 1);
    printf("r = %d, x = %d\n", r, x);

    r = __sync_add_and_fetch(&x, 1);
    printf("r = %d, x = %d\n", r, x);

    r = __sync_bool_compare_and_swap(&x, 1, 5);
    printf("r = %d, x = %d\n", r, x);

    r = __sync_bool_compare_and_swap(&x, 2, 5);
    printf("r = %d, x = %d\n", r, x);

    r = __sync_fetch_and_sub(&x, 1);
    printf("r = %d, x = %d\n", r, x);

    r = __sync_sub_and_fetch(&x, 1);
    printf("r = %d, x = %d\n", r, x);

    return(0);
}

