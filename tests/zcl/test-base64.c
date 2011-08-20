#include <string.h>
#include <stdio.h>

#include <zcl/base64.h>
#include <zcl/memcmp.h>
#include <zcl/test.h>

static int __test_encode (z_test_t *test) {
    char buffer[16];
    unsigned int n;

    n = z_base64_encode(buffer, "", 0);
    if (n != 0)
        return(1);

    n = z_base64_encode(buffer, "f", 1);
    if (n != 4 || z_memcmp(buffer, "Zg==", 4))
        return(2);

    n = z_base64_encode(buffer, "fo", 2);
    if (n != 4 || z_memcmp(buffer, "Zm8=", 4))
        return(3);

    n = z_base64_encode(buffer, "foo", 3);
    if (n != 4 || z_memcmp(buffer, "Zm9v", 4))
        return(4);

    n = z_base64_encode(buffer, "foob", 4);
    if (n != 8 || z_memcmp(buffer, "Zm9vYg==", 8))
        return(5);

    n = z_base64_encode(buffer, "fooba", 5);
    if (n != 8 || z_memcmp(buffer, "Zm9vYmE=", 8))
        return(6);

    n = z_base64_encode(buffer, "foobar", 6);
    if (n != 8 || z_memcmp(buffer, "Zm9vYmFy", 8))
        return(7);

    return(0);
}

static int __test_decode (z_test_t *test) {
    char buffer[16];
    unsigned int n;

    n = z_base64_decode(buffer, "", 0);
    if (n != 0)
        return(1);

    n = z_base64_decode(buffer, "Zg==", 4);
    if (n != 1 || z_memcmp(buffer, "f", 1))
        return(2);

    n = z_base64_decode(buffer, "Zm8=", 4);
    if (n != 2 || z_memcmp(buffer, "fo", 2))
        return(3);

    n = z_base64_decode(buffer, "Zm9v", 4);
    if (n != 3 || z_memcmp(buffer, "foo", 3))
        return(4);

    n = z_base64_decode(buffer, "Zm9vYg==", 8);
    if (n != 4 || z_memcmp(buffer, "foob", 4))
        return(5);

    n = z_base64_decode(buffer, "Zm9vYmE=", 8);
    if (n != 5 || z_memcmp(buffer, "fooba", 5))
        return(6);

    n = z_base64_decode(buffer, "Zm9vYmFy", 8);
    if (n != 6 || z_memcmp(buffer, "foobar", 6))
        return(7);

    return(0);
}

static z_test_t __test_base64 = {
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_encode,
        __test_decode,
        NULL,
    },
};

int main (int argc, char **argv) {
    int res;

    if ((res = z_test_run(&__test_base64, NULL)))
        printf(" [ !! ] Base64 %d\n", res);
    else
        printf(" [ ok ] Base64\n");

    return(res);
}
