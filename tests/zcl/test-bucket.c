#include <stdio.h>

#include <zcl/snprintf.h>
#include <zcl/macros.h>
#include <zcl/memcmp.h>
#include <zcl/bucket.h>
#include <zcl/test.h>

#define __BLOCK_SIZE        (512)
#define __BUCKET_MAGIC      (0xffff)
#define __BUCKET_LEVEL      (0)

static int __bucket_keycmp (void *user_data,
                            const void *key1,
                            unsigned int k1len,
                            const void *key2,
                            unsigned int k2len)
{
    int cmp;

    if ((cmp = z_memcmp(key1, key2, z_min(k1len, k2len))))
        return(cmp);

    return(k1len - k2len);
}

static uint32_t __bucket_csum32 (void *user_data,
                                 const void *data,
                                 unsigned int n)
{
    return(z_csum32_adler(0, data, n));
}

static void __bucket_info_init (z_bucket_info_t *info) {
    info->user_data = NULL;
    info->key_compare = __bucket_keycmp;
    info->crc_func = __bucket_csum32;
    info->size = __BLOCK_SIZE;
}

static int __test_fixed (z_test_t *test) {
    uint8_t block[__BLOCK_SIZE];
    z_bucket_info_t binfo;
    z_bucket_item_t item;
    z_bucket_t bucket;
    uint64_t value;
    uint32_t key;

    __bucket_info_init(&binfo);
    z_bucket_create(&bucket, &z_bucket_fixed, &binfo, block,
                    __BUCKET_MAGIC, __BUCKET_LEVEL, 4, 8);

    for (key = 1, value = 1; key <= 10; ++key, value += 12)
        z_bucket_append(&bucket, &key, 0, &value, 0);

    z_bucket_close(&bucket);

    for (key = 1, value = 1; key <= 10; ++key, value += 12) {
        if (z_bucket_search(&bucket, &item, &key, 0))
            return(1);

        if (item.klength != 4 || item.vlength != 8)
            return(2);

        if (Z_UINT32_PTR_VALUE(item.key) != key)
            return(3);

        if (Z_UINT64_PTR_VALUE(item.value) != value)
            return(4);
    }

    key = 0;
    if (z_bucket_search(&bucket, &item, &key, 0) == 0)
        return(5);

    key = 100;
    if (z_bucket_search(&bucket, &item, &key, 0) == 0)
        return(6);

    return(0);
}

static int __test_variable (z_test_t *test) {
    uint8_t block[__BLOCK_SIZE];
    z_bucket_info_t binfo;
    z_bucket_item_t item;
    uint32_t i, nk, nv;
    z_bucket_t bucket;
    char value[24];
    char key[24];

    __bucket_info_init(&binfo);
    z_bucket_create(&bucket, &z_bucket_variable, &binfo, block,
                    __BUCKET_MAGIC, __BUCKET_LEVEL, 0, 0);

    for (i = 1; i < 100000000; i *= 10) {
        nk = z_snprintf(key, sizeof(key), "K%u4d", i);
        nv = z_snprintf(value, sizeof(value), "V%u4d", i * 10);
        z_bucket_append(&bucket, &key, nk, &value, nv);
    }

    z_bucket_close(&bucket);

    for (i = 1; i < 100000000; i *= 10) {
        nk = z_snprintf(key, sizeof(key), "K%u4d", i);
        nv = z_snprintf(value, sizeof(value), "V%u4d", i * 10);

        if (z_bucket_search(&bucket, &item, &key, nk))
            return(1);

        if (item.klength != nk || item.vlength != nv)
            return(2);

        if (z_memcmp(item.key, key, nk))
            return(3);

        if (z_memcmp(item.value, value, nv))
            return(4);
    }

    if (z_bucket_search(&bucket, &item, "K0", 2) == 0)
        return(5);

    if (z_bucket_search(&bucket, &item, "K10000000000", 12) == 0)
        return(6);

    return(0);
}

static int __test_fixed_key (z_test_t *test) {
    uint8_t block[__BLOCK_SIZE];
    z_bucket_info_t binfo;
    z_bucket_item_t item;
    z_bucket_t bucket;
    uint32_t key, nv;
    char value[24];

    __bucket_info_init(&binfo);
    z_bucket_create(&bucket, &z_bucket_fixed_key, &binfo, block,
                    __BUCKET_MAGIC, __BUCKET_LEVEL, 4, 0);

    for (key = 1; key <= 10; ++key) {
        nv = z_snprintf(value, sizeof(value), "V%u4d", 1 << (key * 2));
        z_bucket_append(&bucket, &key, 0, &value, nv);
    }

    for (key = 1; key <= 10; ++key) {
        nv = z_snprintf(value, sizeof(value), "V%u4d", 1 << (key * 2));

        if (z_bucket_search(&bucket, &item, &key, 0))
            return(1);

        if (item.klength != 4 || item.vlength != nv)
            return(2);

        if (Z_UINT32_PTR_VALUE(item.key) != key)
            return(3);

        if (z_memcmp(item.value, value, nv))
            return(4);
    }

    key = 0;
    if (z_bucket_search(&bucket, &item, &key, 0) == 0)
        return(5);

    key = 100;
    if (z_bucket_search(&bucket, &item, &key, 0) == 0)
        return(6);

    return(0);
}

static int __test_fixed_value (z_test_t *test) {
    uint8_t block[__BLOCK_SIZE];
    z_bucket_info_t binfo;
    z_bucket_item_t item;
    uint32_t i, nk;
    z_bucket_t bucket;
    uint64_t value;
    char key[24];

    __bucket_info_init(&binfo);
    z_bucket_create(&bucket, &z_bucket_fixed_value, &binfo, block,
                    __BUCKET_MAGIC, __BUCKET_LEVEL, 0, 8);

    for (i = 1, value = 1; i < 100000000; i *= 10, value += 12) {
        nk = z_snprintf(key, sizeof(key), "K%u4d", i);
        z_bucket_append(&bucket, &key, nk, &value, 0);
    }

    z_bucket_close(&bucket);

    for (i = 1, value = 1; i < 100000000; i *= 10, value += 12) {
        nk = z_snprintf(key, sizeof(key), "K%u4d", i);

        if (z_bucket_search(&bucket, &item, &key, nk))
            return(1);

        if (item.klength != nk || item.vlength != 8)
            return(2);

        if (z_memcmp(item.key, key, nk))
            return(3);

        if (Z_UINT64_PTR_VALUE(item.value) != value)
            return(4);
    }

    if (z_bucket_search(&bucket, &item, "K0", 2) == 0)
        return(5);

    if (z_bucket_search(&bucket, &item, "K10000000000", 12) == 0)
        return(6);

    return(0);
}

static z_test_t __test_hash = {
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_fixed,
        __test_variable,
        __test_fixed_key,
        __test_fixed_value,
        NULL,
    },
};

int main (int argc, char **argv) {
    int res;

    if ((res = z_test_run(&__test_hash, NULL)))
        printf(" [ !! ] Bucket %d\n", res);
    else
        printf(" [ ok ] Bucket\n");

    return(res);
}

