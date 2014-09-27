#ifndef _Z_AVL_H_
#define _Z_AVL_H_

#include <zcl/macros.h>

typedef int (*z_memcmp_t) (const void *s1, const void *s2, size_t n);


void     z_avl16_init   (uint8_t *block, uint32_t size, uint16_t stride);
void     z_avl16_expand (uint8_t *block, uint32_t size);
uint8_t *z_avl16_append (uint8_t *block, const void *key, uint16_t ksize);
uint8_t *z_avl16_insert (uint8_t *block, z_memcmp_t kcmp, const void *key, uint16_t ksize);
uint8_t *z_avl16_remove (uint8_t *block, z_memcmp_t kcmp, const void *key, uint16_t ksize);
uint8_t *z_avl16_lookup (uint8_t *block, z_memcmp_t kcmp, const void *key, uint16_t ksize);

uint32_t z_avl16_avail    (const uint8_t *block);
uint32_t z_avl16_overhead (const uint8_t *block);
void     z_avl16_dump     (const uint8_t *block);

#endif /* !_Z_AVL_H_ */
