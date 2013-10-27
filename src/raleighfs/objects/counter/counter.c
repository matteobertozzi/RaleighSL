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

#include <zcl/global.h>
#include <zcl/debug.h>

#include "counter.h"

#define RALEIGHFS_COUNTER(x)            Z_CAST(raleighfs_counter_t, x)

typedef struct raleighfs_counter {
  int64_t read_value;
  int64_t write_value;
  uint64_t txn_id;
} raleighfs_counter_t;

/* ============================================================================
 *  PUBLIC Counter READ methods
 */
raleighfs_errno_t raleighfs_counter_get (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t *current_value)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);

  /* Get transaction value */
  if (transaction != NULL && counter->txn_id == raleighfs_txn_id(transaction)) {
    *current_value = counter->write_value;
    return(RALEIGHFS_ERRNO_NONE);
  }

  *current_value = counter->read_value;
  return(RALEIGHFS_ERRNO_NONE);
}

/* ============================================================================
 *  PUBLIC Counter WRITE methods
 */
raleighfs_errno_t raleighfs_counter_set (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t value)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  uint64_t txn_id;

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighfs_txn_id(transaction) : 0;
  if (counter->txn_id > 0 && counter->txn_id != txn_id)
    return(RALEIGHFS_ERRNO_TXN_LOCKED_OPERATION);

  if (transaction != NULL && counter->txn_id != txn_id) {
    raleighfs_errno_t errno;
    if ((errno = raleighfs_transaction_add(fs, transaction, object, NULL)))
      return(errno);
  }

  counter->write_value = value;
  counter->txn_id = txn_id;
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_counter_cas (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t old_value,
                                         int64_t new_value,
                                         int64_t *current_value)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  uint64_t txn_id;

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighfs_txn_id(transaction) : 0;
  if (counter->txn_id > 0 && counter->txn_id != txn_id)
    return(RALEIGHFS_ERRNO_TXN_LOCKED_OPERATION);

  *current_value = counter->write_value;
  if (counter->write_value != old_value)
    return(RALEIGHFS_ERRNO_DATA_CAS);

  if (transaction != NULL && counter->txn_id != txn_id) {
    raleighfs_errno_t errno;
    if ((errno = raleighfs_transaction_add(fs, transaction, object, NULL)))
      return(errno);
  }

  counter->write_value = new_value;
  counter->txn_id = txn_id;
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_counter_add (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t value,
                                         int64_t *current_value)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  uint64_t txn_id;

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighfs_txn_id(transaction) : 0;
  if (counter->txn_id > 0 && counter->txn_id != txn_id)
    return(RALEIGHFS_ERRNO_TXN_LOCKED_OPERATION);

  if (transaction != NULL && counter->txn_id != txn_id) {
    raleighfs_errno_t errno;
    if ((errno = raleighfs_transaction_add(fs, transaction, object, NULL)))
      return(errno);
  }

  counter->write_value += value;
  *current_value = counter->write_value;
  counter->txn_id = txn_id;
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_counter_mul (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int64_t mul,
                                         int64_t *current_value)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  uint64_t txn_id;

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighfs_txn_id(transaction) : 0;
  if (counter->txn_id > 0 && counter->txn_id != txn_id)
    return(RALEIGHFS_ERRNO_TXN_LOCKED_OPERATION);

  if (transaction != NULL && counter->txn_id != txn_id) {
    raleighfs_errno_t errno;
    if ((errno = raleighfs_transaction_add(fs, transaction, object, NULL)))
      return(errno);
  }

  counter->write_value *= mul;
  *current_value = counter->write_value;
  counter->txn_id = txn_id;
  return(RALEIGHFS_ERRNO_NONE);
}

/* ============================================================================
 *  Counter Object Plugin
 */
static raleighfs_errno_t __object_create (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
  raleighfs_counter_t *counter;

  counter = z_memory_struct_alloc(z_global_memory(), raleighfs_counter_t);
  if (Z_MALLOC_IS_NULL(counter))
    return(RALEIGHFS_ERRNO_NO_MEMORY);

  counter->read_value = 0;
  counter->write_value = 0;
  counter->txn_id = 0;

  object->membufs = counter;
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_close (raleighfs_t *fs,
                                         raleighfs_object_t *object)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  z_memory_struct_free(z_global_memory(), raleighfs_counter_t, counter);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_commit (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  if (counter->txn_id == 0) {
    counter->read_value = counter->write_value;
  }
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_rollback (raleighfs_t *fs,
                                            raleighfs_object_t *object)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  if (counter->txn_id == 0) {
    counter->write_value = counter->read_value;
  }
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_apply (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         void *mutation)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  counter->read_value = counter->write_value;
  counter->txn_id = 0;
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_revert (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          void *mutation)
{
  raleighfs_counter_t *counter = RALEIGHFS_COUNTER(object->membufs);
  counter->write_value = counter->read_value;
  counter->txn_id = 0;
  return(RALEIGHFS_ERRNO_NONE);
}

const raleighfs_object_plug_t raleighfs_object_counter = {
  .info = {
    .type = RALEIGHFS_PLUG_TYPE_OBJECT,
    .description = "Counter Object",
    .label       = "counter",
  },

  .create   = __object_create,
  .open     = NULL,
  .close    = __object_close,
  .commit   = __object_commit,
  .rollback = __object_rollback,
  .sync     = NULL,
  .unlink   = NULL,
  .apply    = __object_apply,
  .revert   = __object_revert,
};

