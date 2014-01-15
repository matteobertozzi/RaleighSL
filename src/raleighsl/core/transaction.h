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

#ifndef _RALEIGHSL_TRANSACTION_H_
#define _RALEIGHSL_TRANSACTION_H_

#include <raleighsl/types.h>

raleighsl_transaction_t *raleighsl_transaction_alloc (raleighsl_t *fs,
                                                      uint64_t txn_id);
void                     raleighsl_transaction_free  (raleighsl_t *fs,
                                                      raleighsl_transaction_t *txn);

raleighsl_errno_t raleighsl_transaction_create   (raleighsl_t *fs,
                                                  uint64_t *txn_id);

raleighsl_errno_t raleighsl_transaction_acquire (raleighsl_t *fs,
                                                 uint64_t txn_id,
                                                 raleighsl_transaction_t **txn);
void              raleighsl_transaction_release (raleighsl_t *fs,
                                                 raleighsl_transaction_t *txn);

raleighsl_errno_t raleighsl_transaction_add     (raleighsl_t *fs,
                                                 raleighsl_transaction_t *transaction,
                                                 raleighsl_object_t *object,
                                                 raleighsl_txn_atom_t *atom);
void              raleighsl_transaction_replace (raleighsl_t *fs,
                                                 raleighsl_transaction_t *transaction,
                                                 raleighsl_object_t *object,
                                                 raleighsl_txn_atom_t *atom,
                                                 raleighsl_txn_atom_t *new_atom);
void              raleighsl_transaction_remove  (raleighsl_t *fs,
                                                 raleighsl_transaction_t *transaction,
                                                 raleighsl_object_t *object,
                                                 raleighsl_txn_atom_t *atom);

#endif /* !_RALEIGHSL_TRANSACTION_H_ */
