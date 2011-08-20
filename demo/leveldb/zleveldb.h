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

#ifndef _Z_LEVELDB_H_
#define _Z_LEVELDB_H_

#include <leveldb/c.h>

#define Z_LEVELDB(x)                ((z_leveldb_t *)(x))

typedef struct z_leveldb {
    leveldb_writeoptions_t *woptions;
    leveldb_readoptions_t *roptions;
    leveldb_options_t *options;
    leveldb_comparator_t *cmp;
    leveldb_cache_t *cache;
    leveldb_env_t *env;
    leveldb_t *db;
    char *error;
} z_leveldb_t;

int z_leveldb_open (z_leveldb_t *leveldb,
                    const char *name,
                    size_t cache_capacity);
void z_leveldb_close (z_leveldb_t *leveldb);

const char *z_leveldb_error (z_leveldb_t *leveldb);
void z_leveldb_print_error (z_leveldb_t *leveldb);
char *z_leveldb_stats (z_leveldb_t *leveldb);
leveldb_iterator_t *z_leveldb_iter (z_leveldb_t *leveldb);

int z_leveldb_write (z_leveldb_t *leveldb,
                     leveldb_writebatch_t *wbatch);

int z_leveldb_put (z_leveldb_t *leveldb,
                   const void *key,
                   size_t keylen,
                   const void *value,
                   size_t vallen);

void *z_leveldb_get (z_leveldb_t *leveldb,
                     const void *key,
                     size_t keylen,
                     size_t *vallen);

int z_leveldb_delete (z_leveldb_t *leveldb,
                      const void *key,
                      size_t keylen);

#endif /* !_Z_LEVELDB_H_ */

