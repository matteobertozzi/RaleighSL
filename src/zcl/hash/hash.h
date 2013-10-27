/*
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

#ifndef _Z_HASH_H_
#define _Z_HASH_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/opaque.h>
#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_hash32_plug)
Z_TYPEDEF_STRUCT(z_hash_plug)

Z_TYPEDEF_STRUCT(z_hash160)
Z_TYPEDEF_STRUCT(z_hash32)
Z_TYPEDEF_STRUCT(z_hash)

#define Z_HASH32(x)                        Z_CAST(z_hash32_t, x)
#define Z_HASH(x)                          Z_CAST(z_hash_t, x)

typedef uint32_t    (*z_hash32_func_t)     (const void *blob,
                                            unsigned int n,
                                            uint32_t seed);

typedef uint64_t (*z_hash_func_t) (void *udata, const void *obj, uint32_t seed);

struct z_hash_plug {
  int     (*init)     (z_hash_t *hash);
  void    (*uninit)   (z_hash_t *hash);
  void    (*update)   (z_hash_t *hash,
                       const void *blob,
                       unsigned int n);
  void    (*digest)   (z_hash_t *hash,
                       void *digest);
};

struct z_hash32_plug {
  void    (*init)     (z_hash32_t *hash,
                       uint32_t seed);
  void    (*update)   (z_hash32_t *hash,
                       const void *blob,
                       unsigned int n);
  void    (*digest)   (z_hash32_t *hash);
};

struct z_hash {
  const z_hash_plug_t *plug;
  z_memory_t *   memory;
  z_opaque_t     plug_data;
};

struct z_hash32 {
  const z_hash32_plug_t *plug;
  z_hash32_func_t  func;
  uint8_t          buffer[32];
  unsigned int     bufsize;
  unsigned int     length;
  uint32_t         hash;
};

extern const z_hash_plug_t z_hash160_plug_sha;
extern const z_hash_plug_t z_hash256_plug_sha;
extern const z_hash32_plug_t z_hash32_plug_murmur3;

int             z_hash_alloc            (z_hash_t *hash,
                                         z_memory_t *memory,
                                         const z_hash_plug_t *plug);
void            z_hash_free             (z_hash_t *hash);
void            z_hash_update           (z_hash_t *hash,
                                         const void *blob,
                                         unsigned int n);
void            z_hash_digest           (z_hash_t *hash,
                                         void *digest);

void            z_hash32_init           (z_hash32_t *hash,
                                         z_hash32_func_t func,
                                         uint32_t seed);
void            z_hash32_init_plug      (z_hash32_t *hash,
                                         z_hash32_plug_t *plug,
                                         uint32_t seed);
void            z_hash32_update         (z_hash32_t *hash,
                                         const void *blob,
                                         unsigned int n);
uint32_t        z_hash32_digest         (z_hash32_t *hash);

char *          z_hash_to_string        (char *buffer,
                                         const void *hash,
                                         unsigned int n);


uint32_t        z_hash32_js             (const void *blob,
                                         unsigned int n,
                                         uint32_t seed);
uint32_t        z_hash32_elfv           (const void *blob,
                                         unsigned int n,
                                         uint32_t seed);
uint32_t        z_hash32_sdbm           (const void *blob,
                                         unsigned int n,
                                         uint32_t seed);
uint32_t        z_hash32_dek            (const void *blob,
                                         unsigned int n,
                                         uint32_t seed);
uint32_t        z_hash32_djb            (const void *data,
                                         unsigned int n,
                                         uint32_t seed);
uint32_t        z_hash32_fnv            (const void *data,
                                         unsigned int n,
                                         uint32_t seed);
uint32_t        z_hash32_string         (const void *blob,
                                         unsigned int n,
                                         uint32_t seed);
uint32_t        z_hash32_murmur3        (const void *blob,
                                         unsigned int n,
                                         uint32_t seed);

uint64_t        z_hash64a               (uint64_t value);

void            z_hash160_sha          (uint8_t hash[20],
                                        const void *data,
                                        unsigned int size);
void            z_hash160_ripemd       (uint8_t hash[20],
                                        const void *data,
                                        unsigned int size);
void            z_hash256_sha          (uint8_t hash[32],
                                        const void *data,
                                        unsigned int size);

__Z_END_DECLS__

#endif /* !_Z_HASH_H_ */
