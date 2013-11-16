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

#ifndef _RALEIGHSL_NUMBER_H_
#define _RALEIGHSL_NUMBER_H_

#include <raleighsl/raleighsl.h>

extern const raleighsl_object_plug_t raleighsl_object_number;

raleighsl_errno_t raleighsl_number_get  (raleighsl_t *fs,
                                         const raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         int64_t *current_value);
raleighsl_errno_t raleighsl_number_set  (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         int64_t value);
raleighsl_errno_t raleighsl_number_cas  (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         int64_t old_value,
                                         int64_t new_value,
                                         int64_t *current_value);
raleighsl_errno_t raleighsl_number_add  (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         int64_t value,
                                         int64_t *current_value);
raleighsl_errno_t raleighsl_number_mul  (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         int64_t mul,
                                         int64_t *current_value);
raleighsl_errno_t raleighsl_number_div  (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         int64_t div,
                                         int64_t *mod,
                                         int64_t *current_value);

#endif /* !_RALEIGHSL_NUMBER_H_ */
