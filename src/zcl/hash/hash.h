#ifndef _Z_HASH_H_
#define _Z_HASH_H_

#include <stdint.h>

uint32_t    z_hash32_js         (const void *data,
                                 unsigned int n,
                                 uint32_t seed);
uint32_t    z_hash32_elf        (const void *data,
                                 unsigned int n,
                                 uint32_t seed);
uint32_t    z_hash32_elfv       (const void *data,
                                 unsigned int n,
                                 uint32_t seed);
uint32_t    z_hash32_sdbm       (const void *data,
                                 unsigned int n,
                                 uint32_t seed);
uint32_t    z_hash32_dek        (const void *data,
                                 unsigned int n,
                                 uint32_t seed);

uint32_t    z_hash32_string     (const void *data,
                                 unsigned int n,
                                 uint32_t seed);

uint32_t    z_hash32_murmur2    (const void *data,
                                 unsigned int n,
                                 uint32_t seed);
uint32_t    z_hash32_murmur3    (const void *data,
                                 unsigned int len,
                                 uint32_t seed);

char *      z_hash_to_string    (char *buffer,
                                 const void *hash,
                                 uint32_t n);

#endif /* !_Z_HASH_H_ */

