#include <stdint.h>

struct pointer16 {
    uint64_t hi;
    uint64_t lo;
};

#define __dwcas(ptr, old, new)                                    \
    __sync_bool_compare_and_swap((__uint128_t *)(ptr),            \
                                 *((__uint128_t *)(old)),         \
                                 *((__uint128_t *)(new)))

int main (int argc, char **argv) {
    struct pointer16 a, b, c;
    a.hi = 1; a.lo = 2;
    b.hi = 1; b.lo = 2;
    c.hi = 2; c.lo = 3;
    if (__dwcas(&a, &b, &c)) {
        return(0);
    }
    return(1);
}
