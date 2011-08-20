/*
 *   Copyright 2011 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "zleveldb.h"

static void __leveldb_destructor (void *arg) {
}

static int __leveldb_compare (void *arg,
                              const char *a, size_t alen,
                              const char *b, size_t blen)
{
    int r;

    if ((r = memcmp(a, b, (alen < blen) ? alen : blen)))
        return(r);

    return((alen < blen) ? -1 : (alen > blen) ? 1 : 0);
}

static const char *__leveldb_name (void *arg) {
    return("z-leveldb");
}

static void __leveldb_reset_error (z_leveldb_t *leveldb) {
    if (leveldb->error != NULL) {
        free(leveldb->error);
        leveldb->error = NULL;
    }
}

int z_leveldb_open (z_leveldb_t *leveldb,
                    const char *name,
                    size_t cache_capacity)
{
    /* Initialize comparator */
    leveldb->cmp = leveldb_comparator_create(NULL,
                                             __leveldb_destructor,
                                             __leveldb_compare,
                                             __leveldb_name);

    /* Initialize env */
    leveldb->env = leveldb_create_default_env();

    /* Initialize options */
    leveldb->options = leveldb_options_create();
    leveldb_options_set_comparator(leveldb->options, leveldb->cmp);
    leveldb_options_set_create_if_missing(leveldb->options, 1);
    leveldb_options_set_error_if_exists(leveldb->options, 0);
    leveldb_options_set_env(leveldb->options, leveldb->env);
    leveldb_options_set_info_log(leveldb->options, NULL);
    leveldb_options_set_write_buffer_size(leveldb->options, 100000);
    leveldb_options_set_paranoid_checks(leveldb->options, 1);
    leveldb_options_set_max_open_files(leveldb->options, 10);
    leveldb_options_set_block_size(leveldb->options, 8192);
    leveldb_options_set_block_restart_interval(leveldb->options, 8);
    leveldb_options_set_compression(leveldb->options, leveldb_snappy_compression);

    /* Initialize cache */
    if (cache_capacity > 0) {
        leveldb->cache = leveldb_cache_create_lru(cache_capacity);
        leveldb_options_set_cache(leveldb->options, leveldb->cache);
    } else {
        leveldb->cache = NULL;
    }

    /* Initialize read options */
    leveldb->roptions = leveldb_readoptions_create();
    leveldb_readoptions_set_verify_checksums(leveldb->roptions, 1);
    leveldb_readoptions_set_fill_cache(leveldb->roptions, 1);

    /* Initialize write options */
    leveldb->woptions = leveldb_writeoptions_create();
    leveldb_writeoptions_set_sync(leveldb->woptions, 1);

    /* Open/Create database */
    leveldb->error = NULL;
    leveldb->db = leveldb_open(leveldb->options, name, &(leveldb->error));

    return(leveldb->db == NULL);
}

void z_leveldb_close (z_leveldb_t *leveldb) {
    __leveldb_reset_error(leveldb);
    leveldb_close(leveldb->db);
    leveldb_options_destroy(leveldb->options);
    leveldb_readoptions_destroy(leveldb->roptions);
    leveldb_writeoptions_destroy(leveldb->woptions);
    if (leveldb->cache != NULL)
        leveldb_cache_destroy(leveldb->cache);
    leveldb_env_destroy(leveldb->env);
    leveldb_comparator_destroy(leveldb->cmp);
}

const char *z_leveldb_error (z_leveldb_t *leveldb) {
    return(leveldb->error);
}

void z_leveldb_print_error (z_leveldb_t *leveldb) {
    if (leveldb->error != NULL)
        printf("leveldb-error: %s\n", leveldb->error);
    else
        printf("leveldb-error: none\n");
}

char *z_leveldb_stats (z_leveldb_t *leveldb) {
    return(leveldb_property_value(leveldb->db, "leveldb.stats"));
}

leveldb_iterator_t *z_leveldb_iter (z_leveldb_t *leveldb) {
    return(leveldb_create_iterator(leveldb->db, leveldb->roptions));
}

int z_leveldb_write (z_leveldb_t *leveldb,
                     leveldb_writebatch_t *wbatch)
{
    __leveldb_reset_error(leveldb);
    leveldb_write(leveldb->db, leveldb->woptions, wbatch, &(leveldb->error));
    return(leveldb->error != NULL);
}


int z_leveldb_put (z_leveldb_t *leveldb,
                   const void *key,
                   size_t keylen,
                   const void *value,
                   size_t vallen)
{
    __leveldb_reset_error(leveldb);
    leveldb_put(leveldb->db, leveldb->woptions,
                (const char *)key, keylen,
                (const char *)value, vallen,
                &(leveldb->error));
    return(leveldb->error != NULL);
}

void *z_leveldb_get (z_leveldb_t *leveldb,
                     const void *key,
                     size_t keylen,
                     size_t *vallen)
{
    void *value;
    __leveldb_reset_error(leveldb);
    value = leveldb_get(leveldb->db, leveldb->roptions,
                        (const char *)key, keylen, vallen,
                        &(leveldb->error));
    return(value);
}

int z_leveldb_delete (z_leveldb_t *leveldb,
                      const void *key,
                      size_t keylen)
{
    __leveldb_reset_error(leveldb);
    leveldb_delete(leveldb->db, leveldb->woptions,
                   (const char *)key, keylen,
                   &(leveldb->error));
    return(leveldb->error != NULL);
}


#if 0
int main (int argc, char **argv) {
    leveldb_iterator_t *iter;
    z_leveldb_t db;
    void *data;
    size_t sz;

    if (z_leveldb_open(&db, "test.db", 0)) {
        z_leveldb_print_error(&db);
        return(1);
    }

    iter = z_leveldb_iter(&db);
    leveldb_iter_seek_to_first(iter);
    while (leveldb_iter_valid(iter)) {
        data = (void *)leveldb_iter_key(iter, &sz);
        printf("KEY: %lu:%s\n", sz, (const char *)data);
        data = (void *)leveldb_iter_key(iter, &sz);
        printf("VAL: %lu:%s\n", sz, (const char *)data);

        leveldb_iter_next(iter);
    }
    leveldb_iter_destroy(iter);

    data = z_leveldb_get(&db, "key0", 4, &sz);
    printf("%lu: %s\n", sz, (const char *)data);

    data = z_leveldb_get(&db, "key1", 4, &sz);
    printf("%lu: %s\n", sz, (const char *)data);


    z_leveldb_put(&db, "key0", 4, "k0-v0", 5);
    z_leveldb_print_error(&db);
    data = z_leveldb_get(&db, "key0", 4, &sz);
    printf("%lu: %s\n", sz, (const char *)data);

    z_leveldb_put(&db, "key0", 4, "k0-v1", 5);
    z_leveldb_print_error(&db);
    data = z_leveldb_get(&db, "key0", 4, &sz);
    printf("%lu: %s\n", sz, (const char *)data);

    z_leveldb_put(&db, "key1", 4, "k1-v0", 5);
    z_leveldb_print_error(&db);
    data = z_leveldb_get(&db, "key1", 4, &sz);
    printf("%lu: %s\n", sz, (const char *)data);

    z_leveldb_delete(&db, "key0", 4);
    z_leveldb_print_error(&db);
    data = z_leveldb_get(&db, "key0", 4, &sz);
    printf("%lu: %s\n", sz, (const char *)data);

    z_leveldb_delete(&db, "key2", 4);
    z_leveldb_print_error(&db);

    leveldb_writebatch_t *wb = leveldb_writebatch_create();
    leveldb_writebatch_put(wb, "foo", 3, "a", 1);
    leveldb_writebatch_clear(wb);
    leveldb_writebatch_put(wb, "bar", 3, "b", 1);
    leveldb_writebatch_put(wb, "box", 3, "c", 1);
    leveldb_writebatch_delete(wb, "bar", 3);
    z_leveldb_write(&db, wb);
    leveldb_writebatch_destroy(wb);
    z_leveldb_print_error(&db);

    data = z_leveldb_get(&db, "foo", 3, &sz);
    printf("%lu: %s\n", sz, (const char *)data);

    data = z_leveldb_get(&db, "bar", 3, &sz);
    printf("%lu: %s\n", sz, (const char *)data);

    data = z_leveldb_get(&db, "box", 3, &sz);
    printf("%lu: %s\n", sz, (const char *)data);

    data = z_leveldb_stats(&db);
    printf("%s\n", (const char *)data);

    z_leveldb_close(&db);

    return(0);
}
#endif

