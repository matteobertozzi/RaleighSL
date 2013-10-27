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

#ifndef _RALEIGHFS_EXEC_H_
#define _RALEIGHFS_EXEC_H_

#include <raleighfs/types.h>

typedef raleighfs_errno_t (*raleighfs_create_func_t) (raleighfs_t *fs,
                                        const raleighfs_object_plug_t *plug,
                                        void *udata, void *req, void *resp);
typedef raleighfs_errno_t (*raleighfs_lookup_func_t)   (raleighfs_t *fs,
                                        void *udata, void *req, void *resp);
typedef raleighfs_errno_t (*raleighfs_modify_func_t) (raleighfs_t *fs,
                                        void *udata, void *req, void *resp);
typedef raleighfs_errno_t (*raleighfs_read_func_t)   (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *udata, void *req, void *resp);
typedef raleighfs_errno_t (*raleighfs_write_func_t)  (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *udata, void *req, void *resp);
typedef void (*raleighfs_notify_func_t) (raleighfs_t *fs,
                                         uint64_t oid, raleighfs_errno_t errno,
                                         void *udata, void *err_data,
                                         void *req, void *resp);

int raleighfs_exec_create (raleighfs_t *fs,
                           const raleighfs_object_plug_t *plug,
                           raleighfs_create_func_t create_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data);
int raleighfs_exec_rename (raleighfs_t *fs,
                           raleighfs_modify_func_t modify_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data);
int raleighfs_exec_unlink (raleighfs_t *fs,
                           raleighfs_modify_func_t modify_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data);
int raleighfs_exec_lookup (raleighfs_t *fs,
                           raleighfs_lookup_func_t lookup_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data);
int raleighfs_exec_read   (raleighfs_t *fs,
                           uint64_t txn_id, uint64_t oid,
                           raleighfs_read_func_t read_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data);
int raleighfs_exec_write  (raleighfs_t *fs,
                           uint64_t txn_id, uint64_t oid,
                           raleighfs_write_func_t write_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data);

int raleighfs_exec_txn_commit  (raleighfs_t *fs,
                                uint64_t txn_id,
                                raleighfs_notify_func_t notify_func,
                                void *req, void *resp,
                                void *udata, void *err_data);
int raleighfs_exec_txn_rollback (raleighfs_t *fs,
                                 uint64_t txn_id,
                                 raleighfs_notify_func_t notify_func,
                                 void *req, void *resp,
                                 void *udata, void *err_data);

#endif /* !_RALEIGHFS_EXEC_H_ */
