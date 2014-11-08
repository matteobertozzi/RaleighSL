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

#ifndef _Z_AVL16S_H_
#define _Z_AVL16S_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_avl16s_iter {
  uint8_t *block;
  uint16_t stack[18];
  uint16_t current;
  uint16_t height;
} z_avl16s_iter_t;

uint32_t z_avl16s_init             (uint8_t *block,
                                    uint32_t size,
                                    uint16_t stride);
void     z_avl16s_expand           (uint8_t *block,
                                    uint32_t size);
void *   z_avl16s_append           (uint8_t *block);
void *   z_avl16s_insert           (uint8_t *block,
                                    z_compare_t key_cmp,
                                    const void *key,
                                    void *udata);
void *   z_avl16s_remove           (uint8_t *block,
                                    z_compare_t key_cmp,
                                    const void *key,
                                    void *udata);
void *   z_avl16s_lookup           (uint8_t *block,
                                    z_compare_t key_cmp,
                                    const void *key,
                                    void *udata);

uint32_t z_avl16s_avail            (const uint8_t *block);

void     z_avl16s_iter_init        (z_avl16s_iter_t *self, uint8_t *block);
void *   z_avl16s_iter_seek_begin  (z_avl16s_iter_t *self);
void *   z_avl16s_iter_seek_end    (z_avl16s_iter_t *self);
void *   z_avl16s_iter_seek_le     (z_avl16s_iter_t *iter,
                                    z_compare_t key_cmp,
                                    const void *key,
                                    void *udata);
void *   z_avl16s_iter_seek_ge     (z_avl16s_iter_t *iter,
                                    z_compare_t key_cmp,
                                    const void *key,
                                    void *udata);
void *   z_avl16s_iter_next        (z_avl16s_iter_t *self);
void *   z_avl16s_iter_prev        (z_avl16s_iter_t *self);

__Z_END_DECLS__

#endif /* !_Z_AVL16S_H_ */
