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

#include <stdlib.h>

#include <raleighsl/errno.h>

#define __SET_MSG(msg)                                            \
  z_byte_slice_set(slice, msg, sizeof(msg) - 1);                  \
  return(msg)

#define __ERR(x, msg)            case RALEIGHSL_ERRNO_ ## x: __SET_MSG(msg)

#define __ERR_PLUGIN(x, msg)     __ERR(PLUGIN_ ## x, msg)
#define __ERR_OBJECT(x, msg)     __ERR(OBJECT_ ## x, msg)
#define __ERR_NUMBER(x, msg)     __ERR(NUMBER_ ## x, msg)
#define __ERR_DATA(x, msg)       __ERR(DATA_ ## x, msg)
#define __ERR_TXN(x, msg)        __ERR(TXN_ ## x, msg)

const char *raleighsl_errno_byte_slice (raleighsl_errno_t errno,
                                        z_byte_slice_t *slice)
{
  switch (errno) {
    __ERR(NONE, "no error");
    __ERR(NOT_IMPLEMENTED, "not implmented");

    __ERR(SCHED_YIELD, "retry");

    /* System related */
    __ERR(NO_MEMORY, "no memory available");

    /* Plugins related */
    __ERR_PLUGIN(NOT_LOADED, "plugin not loaded");

    /* Transaction related */
    __ERR_TXN(CLOSED, "transaction already closed");
    __ERR_TXN(NOT_FOUND, "invalid txn-id");
    __ERR_TXN(LOCKED_KEY, "another transaction is locking the key");
    __ERR_TXN(LOCKED_OPERATION, "another transaction is locking the operation");
    __ERR_TXN(ROLLEDBACK, "rolledback due to errors");

    /* Semantic related */
    __ERR_OBJECT(EXISTS, "object already exists");
    __ERR_OBJECT(NOT_FOUND, "object not found");

    /* Object related */
    __ERR_OBJECT(WRONG_TYPE, "wrong object type");

    /* Object Data related */
    __ERR_DATA(CAS, "cas failed");
    __ERR_DATA(KEY_EXISTS, "key already exists");
    __ERR_DATA(KEY_NOT_FOUND, "key not found");
    __ERR_DATA(NO_ITEMS, "no items available");

    /* Number related */
    __ERR_NUMBER(DIVMOD_BYZERO, "division or modulo by zero");

    /* Device related */
    /* Format related */
    /* Space related */
    /* Key related */
    default:
      __SET_MSG("unknown error");
  }
  return(NULL);
}

const char *raleighsl_errno_string (raleighsl_errno_t errno) {
  z_byte_slice_t slice;
  return(raleighsl_errno_byte_slice(errno, &slice));
}
