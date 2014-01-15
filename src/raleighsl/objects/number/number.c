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

#include "number.h"

#define RALEIGHSL_NUMBER(x)            Z_CAST(raleighsl_number_t, x)

typedef struct raleighsl_number {
  raleighsl_txn_atom_t __txn_atom__;
  int64_t read_value;
  int64_t write_value;
  uint64_t txn_id;
} raleighsl_number_t;

/* ============================================================================
 *  PUBLIC Number READ methods
 */
raleighsl_errno_t raleighsl_number_get (raleighsl_t *fs,
                                        const raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        int64_t *current_value)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);

  /* Get transaction value */
  if (transaction != NULL && number->txn_id == raleighsl_txn_id(transaction)) {
    *current_value = number->write_value;
    return(RALEIGHSL_ERRNO_NONE);
  }

  *current_value = number->read_value;
  return(RALEIGHSL_ERRNO_NONE);
}

/* ============================================================================
 *  PUBLIC Number WRITE methods
 */
raleighsl_errno_t raleighsl_number_set (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        int64_t value)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  uint64_t txn_id;

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighsl_txn_id(transaction) : 0;
  if (number->txn_id > 0 && number->txn_id != txn_id)
    return(RALEIGHSL_ERRNO_TXN_LOCKED_OPERATION);

  if (transaction != NULL && number->txn_id != txn_id) {
    raleighsl_errno_t errno;
    if ((errno = raleighsl_transaction_add(fs, transaction, object, &(number->__txn_atom__))))
      return(errno);
  }

  number->write_value = value;
  number->txn_id = txn_id;
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_number_cas (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        int64_t old_value,
                                        int64_t new_value,
                                        int64_t *current_value)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  uint64_t txn_id;

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighsl_txn_id(transaction) : 0;
  if (number->txn_id > 0 && number->txn_id != txn_id)
    return(RALEIGHSL_ERRNO_TXN_LOCKED_OPERATION);

  *current_value = number->write_value;
  if (number->write_value != old_value)
    return(RALEIGHSL_ERRNO_DATA_CAS);

  if (transaction != NULL && number->txn_id != txn_id) {
    raleighsl_errno_t errno;
    if ((errno = raleighsl_transaction_add(fs, transaction, object, &(number->__txn_atom__))))
      return(errno);
  }

  number->write_value = new_value;
  number->txn_id = txn_id;
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_number_add (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        int64_t value,
                                        int64_t *current_value)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  uint64_t txn_id;

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighsl_txn_id(transaction) : 0;
  if (number->txn_id > 0 && number->txn_id != txn_id)
    return(RALEIGHSL_ERRNO_TXN_LOCKED_OPERATION);

  if (transaction != NULL && number->txn_id != txn_id) {
    raleighsl_errno_t errno;
    if ((errno = raleighsl_transaction_add(fs, transaction, object, &(number->__txn_atom__))))
      return(errno);
  }

  number->write_value += value;
  *current_value = number->write_value;
  number->txn_id = txn_id;
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_number_mul (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        int64_t mul,
                                        int64_t *current_value)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  uint64_t txn_id;

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighsl_txn_id(transaction) : 0;
  if (number->txn_id > 0 && number->txn_id != txn_id)
    return(RALEIGHSL_ERRNO_TXN_LOCKED_OPERATION);

  if (transaction != NULL && number->txn_id != txn_id) {
    raleighsl_errno_t errno;
    if ((errno = raleighsl_transaction_add(fs, transaction, object, &(number->__txn_atom__))))
      return(errno);
  }

  number->write_value *= mul;
  *current_value = number->write_value;
  number->txn_id = txn_id;
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_number_div (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        int64_t divy,
                                        int64_t *mody,
                                        int64_t *current_value)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  uint64_t txn_id;

  if (divy == 0)
    return(RALEIGHSL_ERRNO_NUMBER_DIVMOD_BYZERO);

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighsl_txn_id(transaction) : 0;
  if (number->txn_id > 0 && number->txn_id != txn_id)
    return(RALEIGHSL_ERRNO_TXN_LOCKED_OPERATION);

  if (transaction != NULL && number->txn_id != txn_id) {
    raleighsl_errno_t errno;
    if ((errno = raleighsl_transaction_add(fs, transaction, object, &(number->__txn_atom__))))
      return(errno);
  }

  *mody = number->write_value % divy;
  number->write_value /= divy;
  *current_value = number->write_value;
  number->txn_id = txn_id;
  return(RALEIGHSL_ERRNO_NONE);
}

/* ============================================================================
 *  Number Object Plugin
 */
static raleighsl_errno_t __object_create (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_number_t *number;

  number = z_memory_struct_alloc(z_global_memory(), raleighsl_number_t);
  if (Z_MALLOC_IS_NULL(number))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  number->read_value = 0;
  number->write_value = 0;
  number->txn_id = 0;

  object->membufs = number;
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_close (raleighsl_t *fs,
                                         raleighsl_object_t *object)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  z_memory_struct_free(z_global_memory(), raleighsl_number_t, number);
  return(RALEIGHSL_ERRNO_NONE);
}

static void __object_apply (raleighsl_t *fs,
                            raleighsl_object_t *object,
                            raleighsl_txn_atom_t *atom)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  Z_ASSERT(atom == &(number->__txn_atom__), "Wrong TXN atom");
  number->read_value = number->write_value;
  number->txn_id = 0;
}

static void __object_revert (raleighsl_t *fs,
                             raleighsl_object_t *object,
                             raleighsl_txn_atom_t *atom)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  Z_ASSERT(atom == &(number->__txn_atom__), "Wrong TXN atom");
  number->write_value = number->read_value;
  number->txn_id = 0;
}

static raleighsl_errno_t __object_commit (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_number_t *number = RALEIGHSL_NUMBER(object->membufs);
  if (number->txn_id == 0) {
    number->read_value = number->write_value;
  }
  return(RALEIGHSL_ERRNO_NONE);
}

const raleighsl_object_plug_t raleighsl_object_number = {
  .info = {
    .type = RALEIGHSL_PLUG_TYPE_OBJECT,
    .description = "Number Object",
    .label       = "number",
  },

  .create   = __object_create,
  .open     = NULL,
  .close    = __object_close,
  .unlink   = NULL,

  .apply    = __object_apply,
  .revert   = __object_revert,
  .commit   = __object_commit,

  .balance  = NULL,
  .sync     = NULL,
};
