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

#ifndef _RALEIGHSL_EXEC_H_
#define _RALEIGHSL_EXEC_H_

#include <raleighsl/types.h>

typedef raleighsl_errno_t (*raleighsl_create_func_t) (raleighsl_t *fs,
                                        const raleighsl_object_plug_t *plug,
                                        void *udata);
typedef raleighsl_errno_t (*raleighsl_lookup_func_t)   (raleighsl_t *fs,
                                        void *udata);
typedef raleighsl_errno_t (*raleighsl_modify_func_t) (raleighsl_t *fs,
                                        void *udata);
typedef raleighsl_errno_t (*raleighsl_read_func_t)   (raleighsl_t *fs,
                                        const raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        void *udata);
typedef raleighsl_errno_t (*raleighsl_write_func_t)  (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        void *udata);
typedef void (*raleighsl_notify_func_t) (raleighsl_t *fs,
                                         uint64_t oid, raleighsl_errno_t errno,
                                         void *udata, void *err_data);

int raleighsl_exec_create (raleighsl_t *fs,
                           const raleighsl_object_plug_t *plug,
                           raleighsl_create_func_t create_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data);
int raleighsl_exec_rename (raleighsl_t *fs,
                           raleighsl_modify_func_t modify_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data);
int raleighsl_exec_unlink (raleighsl_t *fs,
                           raleighsl_modify_func_t modify_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data);
int raleighsl_exec_lookup (raleighsl_t *fs,
                           raleighsl_lookup_func_t lookup_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data);
int raleighsl_exec_read   (raleighsl_t *fs,
                           uint64_t txn_id, uint64_t oid,
                           raleighsl_read_func_t read_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data);
int raleighsl_exec_write  (raleighsl_t *fs,
                           uint64_t txn_id, uint64_t oid,
                           raleighsl_write_func_t write_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data);

int raleighsl_exec_txn_commit   (raleighsl_t *fs,
                                 uint64_t txn_id,
                                 raleighsl_notify_func_t notify_func,
                                 void *udata, void *err_data);
int raleighsl_exec_txn_rollback (raleighsl_t *fs,
                                 uint64_t txn_id,
                                 raleighsl_notify_func_t notify_func,
                                 void *udata, void *err_data);

#endif /* !_RALEIGHSL_EXEC_H_ */
