#include <string.h>
#include <stdio.h>

#include <zcl/bloomfilter.h>
#include <zcl/strlen.h>
#include <zcl/hash.h>
#include <zcl/test.h>

static const char *__test_keys[] = {
    "66269d0fda485a1bd32801fc6ec4f53eea2faa08",
    "508a6ccc545b32641fe7311048defe7cf599ada3",
    "bdd1dbcaab4ea943b8cc1f006823b733762797c4",
    "c98792fc50d7a7d071f1e6e76c0f5b09c778e886",
    "40f1bf0bc332accf1801abfbb5c3367c3b965a8d",
    "7dfc52e314ed9d727e89ccd660022fc55e58e5a3",
    "a052f59393c1867ec35f68f676f45c64ab4fad5c",
    "e526f520264924744a76aa2a73e1325ac45083f0",
    "25fd869f62343eb80bdc9a7ed51ef7a040b35de1",
    "c26e349836949d27709ba89aa16a0f9ae285acdc",
    "407e27870295e8c497cb2a1f7f16cdce5ca6c6d8",
    "96757c4fbcc982909147deab8d85586d0cc5ea68",
    "d87eca86e6d7777b7b53251024e7b793de9296e1",
    "03f3631977d39c2ca6f9b2c377554f4adc801e20",
    "4054f03020b82160bef3e9ab7d3efdebd06dde49",
    "df1c255cdf73a58f81b4e5e32f48b9fdd8945d36",
    "a4f761a60e9af58dc4922d1dc5dec296deab0008",
    "35a0c8e2c6cb6814f506b64fea0864c0290fd790",
    "deb852b7ea56566a7a8988ee47f37f2e4e63c770",
    NULL,
};

struct user_data {
    z_memory_t       memory;
    z_bloom_filter_t filter;
};

static int __filter_hash (void *user_data,
                          const void *key,
                          unsigned int *hashes,
                          unsigned int nhash)
{
    unsigned int key_length;
    unsigned int i;

    key_length = z_strlen((const char *)key);
    for (i = 0; i < nhash; ++i)
        hashes[i] = z_hash32_string(key, key_length, i);

    return(0);
}

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (!z_bloom_filter_alloc(&(data->filter), &(data->memory),
                              __filter_hash, 64, 3, 32, NULL))
    {
        return(1);
    }

    return(0);
}

static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_bloom_filter_free(&(data->filter));
    return(0);
}

static int __test_human (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (z_bloom_filter_contains(&(data->filter), "Key 1"))
        return(1);

    if (z_bloom_filter_contains(&(data->filter), "key:25092011 2"))
        return(2);

    if (z_bloom_filter_add(&(data->filter), "Key 1"))
        return(3);

    if (!z_bloom_filter_contains(&(data->filter), "Key 1"))
        return(4);

    if (z_bloom_filter_add(&(data->filter), "Key 2"))
        return(5);

    if (!z_bloom_filter_contains(&(data->filter), "Key 2"))
        return(6);

    if (z_bloom_filter_add(&(data->filter), "key:25092011 2"))
        return(7);

    if (!z_bloom_filter_contains(&(data->filter), "key:25092011 2"))
        return(8);

    z_bloom_filter_clear(&(data->filter));
    if (z_bloom_filter_contains(&(data->filter), "Key 1"))
        return(8);

    return(0);
}

static int __test_machine (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    const char **p;

    for (p = __test_keys; *p != NULL; ++p) {
        if (z_bloom_filter_add(&(data->filter), *p))
            return(1);
    }

    for (p = __test_keys; *p != NULL; ++p) {
        if (!z_bloom_filter_contains(&(data->filter), *p))
            return(2);
    }

    z_bloom_filter_clear(&(data->filter));
    for (p = __test_keys; *p != NULL; ++p) {
        if (z_bloom_filter_contains(&(data->filter), *p))
            return(2);
    }

    return(0);
}

static z_test_t __test_skip_list = {
    .setup      = __test_setup,
    .tear_down  = __test_tear_down,
    .funcs      = {
        __test_human,
        __test_machine,
        NULL,
    },
};

int main (int argc, char **argv) {
    struct user_data data;
    int res;

    z_memory_init(&(data.memory), z_system_allocator());
    if ((res = z_test_run(&__test_skip_list, &data)))
        printf(" [ !! ] Bloom Filter %d\n", res);
    else
        printf(" [ ok ] Bloom Filter\n");

    printf("        - z_bloom_filter_t      %ubytes\n", (unsigned int)sizeof(z_bloom_filter_t));

    return(0);
}
