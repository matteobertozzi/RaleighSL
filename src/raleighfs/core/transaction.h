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

#ifndef _RALEIGHFS_TRANSACTION_H_
#define _RALEIGHFS_TRANSACTION_H_

#include <raleighfs/types.h>

raleighfs_errno_t raleighfs_transaction_create   (raleighfs_t *fs,
                                                  uint64_t *txn_id);

raleighfs_errno_t raleighfs_transaction_acquire (raleighfs_t *fs,
                                                 uint64_t txn_id,
                                                 raleighfs_transaction_t **txn);
void              raleighfs_transaction_release (raleighfs_t *fs,
                                                 raleighfs_transaction_t *txn);

raleighfs_errno_t raleighfs_transaction_add (raleighfs_t *fs,
                                             raleighfs_transaction_t *transaction,
                                             raleighfs_object_t *object,
                                             void *mutation);

#endif /* !_RALEIGHFS_TRANSACTION_H_ */
