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

#ifndef _RALEIGHFS_COUNTER_H_
#define _RALEIGHFS_COUNTER_H_

#include <raleighfs/raleighfs.h>

extern const raleighfs_object_plug_t raleighfs_object_counter;

raleighfs_errno_t raleighfs_counter_get (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t *current_value);
raleighfs_errno_t raleighfs_counter_set (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t value);
raleighfs_errno_t raleighfs_counter_cas (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t old_value,
                                         int64_t new_value,
                                         int64_t *current_value);
raleighfs_errno_t raleighfs_counter_add (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t value,
                                         int64_t *current_value);
raleighfs_errno_t raleighfs_counter_mul (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t mul,
                                         int64_t *current_value);

#endif /* !_RALEIGHFS_COUNTER_H_ */
