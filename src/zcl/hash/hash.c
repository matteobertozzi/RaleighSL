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

#include <zcl/hash.h>

z_hash_t *z_hash_alloc (z_hash_t *hash,
                        z_memory_t *memory,
                        z_hash_plug_t *plug)
{
    if ((hash = z_object_alloc(memory, hash, z_hash_t)) == NULL)
        return(NULL);

    hash->plug = plug;
    if (plug->init != NULL) {
        if (plug->init(hash)) {
            z_object_free(hash);
            return(NULL);
        }
    }

    return(hash);
}

void z_hash_free (z_hash_t *hash) {
    if (hash->plug->uninit != NULL)
        hash->plug->uninit(hash);
    z_object_free(hash);
}

void z_hash_update (z_hash_t *hash, const void *blob, unsigned int n) {
    hash->plug->update(hash, blob, n);
}

void z_hash_digest (z_hash_t *hash, void *digest) {
    hash->plug->digest(hash, digest);
}

void z_hash32_init (z_hash32_t *hash, z_hash32_func_t func, uint32_t seed) {
    hash->func = func;
    hash->plug = NULL;
    hash->hash = seed;
    hash->bufsize = 0;
    hash->length = 0;

    if (hash->func == z_hash32_murmur3)
        hash->plug = &z_hash32_plug_murmur3;
    else if (hash->func == z_hash32_jenkin)
        hash->plug = &z_hash32_plug_jenkin;

    if (hash->plug != NULL && hash->plug->init != NULL)
        hash->plug->init(hash, seed);
}

void z_hash32_init_plug (z_hash32_t *hash,
                         z_hash32_plug_t *plug,
                         uint32_t seed)
{
    hash->plug = plug;
    hash->hash = seed;
    hash->bufsize = 0;
    hash->length = 0;

    if (hash->plug != NULL && hash->plug->init != NULL)
        hash->plug->init(hash, seed);
}

void z_hash32_update (z_hash32_t *hash, const void *blob, unsigned int n) {
    hash->length += n;

    if (hash->plug != NULL)
        hash->plug->update(hash, blob, n);
    else
        hash->hash = hash->func(blob, n, hash->hash);
}

uint32_t z_hash32_digest (z_hash32_t *hash) {
    if (hash->plug != NULL)
        hash->plug->digest(hash);
    return(hash->hash);
}


char *z_hash_to_string (char *buffer, const void *hash, unsigned int n) {
    const uint8_t *hp = (const uint8_t *)hash;
    char *bp = buffer;
    int c;

    while (n--) {
        c = ((*hp >> 4) & 15);
        *bp++ = (c > 9) ? ((c - 10) + 'a') : (c + '0');

        c = ((*hp) & 15);
        *bp++ = (c > 9) ? ((c - 10) + 'a') : (c + '0');

        hp++;
    }
    *bp = '\0';

    return(buffer);
}

