#include <stdio.h>

#include <zcl/memcmp.h>
#include <zcl/hash.h>
#include <zcl/test.h>

struct user_data {
    z_memory_t memory;
};

static int __test_sha1 (z_test_t *test) {
    uint8_t digest[20];
    char xdigest[40];

    z_hash160_sha1(digest, "", 0);
    z_hash_to_string(xdigest, digest, 20);
    if (z_memcmp(xdigest, "da39a3ee5e6b4b0d3255bfef95601890afd80709", 20))
        return(1);

    z_hash160_sha1(digest, "abc", 3);
    z_hash_to_string(xdigest, digest, 20);
    if (z_memcmp(xdigest, "a9993e364706816aba3e25717850c26c9cd0d89d", 20))
        return(2);

    z_hash160_sha1(digest, "The quick brown fox jumps over the lazy dog", 43);
    z_hash_to_string(xdigest, digest, 20);
    if (z_memcmp(xdigest, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12", 20))
        return(3);

    return(0);
}

static int __test_sha1_plug (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    uint8_t digest[20];
    char xdigest[40];
    z_hash_t hash;

    z_hash_alloc(&hash, &(data->memory), &z_hash160_plug_sha1);
    z_hash_update(&hash, "The quick brown fox ", 20);
    z_hash_update(&hash, "jumps over ", 11);
    z_hash_update(&hash, "the lazy dog", 12);
    z_hash_digest(&hash, digest);
    z_hash_free(&hash);

    z_hash_to_string(xdigest, digest, 20);
    if (z_memcmp(xdigest, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12", 20))
        return(1);

    return(0);
}

static int __test_ripemd160 (z_test_t *test) {
    uint8_t digest[20];
    char xdigest[40];

    z_hash160_ripemd(digest, "", 0);
    z_hash_to_string(xdigest, digest, 20);
    if (z_memcmp(xdigest, "9c1185a5c5e9fc54612808977ee8f548b2258d31", 20))
        return(1);

    z_hash160_ripemd(digest, "abc", 3);
    z_hash_to_string(xdigest, digest, 20);
    if (z_memcmp(xdigest, "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc", 20))
        return(2);

    z_hash160_ripemd(digest, "The quick brown fox jumps over the lazy dog", 43);
    z_hash_to_string(xdigest, digest, 20);
    if (z_memcmp(xdigest, "37f332f68db77bd9d7edd4969571ad671cf9dd3b", 20))
        return(3);

    return(0);
}

static int __test_ripemd160_plug (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    uint8_t digest[20];
    char xdigest[40];
    z_hash_t hash;

    z_hash_alloc(&hash, &(data->memory), &z_hash160_plug_ripemd);
    z_hash_update(&hash, "The quick brown fox ", 20);
    z_hash_update(&hash, "jumps over ", 11);
    z_hash_update(&hash, "the lazy dog", 12);
    z_hash_digest(&hash, digest);
    z_hash_free(&hash);

    z_hash_to_string(xdigest, digest, 20);
    if (z_memcmp(xdigest, "37f332f68db77bd9d7edd4969571ad671cf9dd3b", 20))
        return(1);

    return(0);
}

static int __test_hash32_plug (z_test_t *test) {
    unsigned int o32;
    unsigned int h32;
    z_hash32_t hash;

    o32 = z_hash32_murmur2("The quick brown fox ", 20, 0xf5);
    o32 = z_hash32_murmur2("jumps over ", 11, o32);
    o32 = z_hash32_murmur2("the lazy dog", 12, o32);

    z_hash32_init(&hash, z_hash32_murmur2, 0xf5);
    z_hash32_update(&hash, "The quick brown fox ", 20);
    z_hash32_update(&hash, "jumps over ", 11);
    z_hash32_update(&hash, "the lazy dog", 12);
    h32 = z_hash32_digest(&hash);

    if (o32 != h32)
        return(1);

    return(0);
}

static z_test_t __test_hash = {
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_sha1,
        __test_sha1_plug,
        __test_ripemd160,
        __test_ripemd160_plug,
        __test_hash32_plug,
        NULL,
    },
};

int main (int argc, char **argv) {
    struct user_data data;
    int res;

    z_memory_init(&(data.memory), z_system_allocator());

    if ((res = z_test_run(&__test_hash, &data)))
        printf(" [ !! ] Hash %d\n", res);
    else
        printf(" [ ok ] Hash\n");

    return(res);
}
