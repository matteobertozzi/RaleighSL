/*
 *   Copyright 2007-2013 Matteo Bertozzi
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

#ifndef _RALEIGHSL_FLOW_H_
#define _RALEIGHSL_FLOW_H_

#include <raleighsl/raleighsl.h>
#include <zcl/bytesref.h>

extern const raleighsl_object_plug_t raleighsl_object_flow;


raleighsl_errno_t raleighsl_flow_append   (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           const z_bytes_ref_t *data,
                                           uint64_t *res_size);
raleighsl_errno_t raleighsl_flow_inject   (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           const z_bytes_ref_t *data,
                                           uint64_t *res_size);
raleighsl_errno_t raleighsl_flow_write    (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           const z_bytes_ref_t *data,
                                           uint64_t *res_size);
raleighsl_errno_t raleighsl_flow_remove   (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           uint64_t size,
                                           uint64_t *res_size);
raleighsl_errno_t raleighsl_flow_truncate (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t size,
                                           uint64_t *res_size);
raleighsl_errno_t raleighsl_flow_read     (raleighsl_t *fs,
                                           const raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           uint64_t size,
                                           z_bytes_ref_t *data);

#endif /* !_RALEIGHSL_FLOW_H_ */
