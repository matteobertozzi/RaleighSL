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

#ifndef _Z_AVL16_COW_H_
#define _Z_AVL16_COW_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_avl16_txn z_avl16_txn_t;

struct z_avl16_txn {
  uint64_t seqid;
  uint16_t root;
  uint8_t  dirty  : 1;
  uint8_t  failed : 1;
  uint8_t  __pad  : 6;
} __attribute__((packed));

uint32_t  z_avl16cow_init       (uint8_t *block, uint32_t size, uint16_t stride);
int       z_avl16cow_clean      (uint8_t *block, uint64_t seqid);
int       z_avl16cow_clean_all  (uint8_t *block);
int       z_avl16_txn_open      (z_avl16_txn_t *self, uint8_t *block, uint64_t seqid);
void      z_avl16_txn_revert    (z_avl16_txn_t *self, uint8_t *block);
int       z_avl16_txn_commit    (z_avl16_txn_t *self, uint8_t *block);
void      z_avl16_txn_dump      (z_avl16_txn_t *self, uint8_t *block);

void *    z_avl16_txn_append    (z_avl16_txn_t *self, uint8_t *block);
void *    z_avl16_txn_insert    (z_avl16_txn_t *self,
                                 uint8_t *block,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);
void *    z_avl16_txn_remove    (z_avl16_txn_t *self,
                                 uint8_t *block,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);

void *    z_avl16_txn_lookup    (z_avl16_txn_t *self,
                                 uint8_t *block,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);

__Z_END_DECLS__

#endif /* !_Z_AVL16_COW_H_ */
