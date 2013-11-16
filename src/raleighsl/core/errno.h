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

#ifndef _RALEIGHSL_ERRNO_H_
#define _RALEIGHSL_ERRNO_H_

#include <zcl/byteslice.h>

typedef enum raleighsl_errno {
  /* Info */
  RALEIGHSL_ERRNO_NONE,
  RALEIGHSL_ERRNO_NOT_IMPLEMENTED,
  RALEIGHSL_ERRNO_SCHED_YIELD,

  /* System related */
  RALEIGHSL_ERRNO_NO_MEMORY,

  /* Plugins related */
  RALEIGHSL_ERRNO_PLUGIN_NOT_LOADED,

  /* Transaction related */
  RALEIGHSL_ERRNO_TXN_CLOSED,
  RALEIGHSL_ERRNO_TXN_NOT_FOUND,
  RALEIGHSL_ERRNO_TXN_ROLLEDBACK,
  RALEIGHSL_ERRNO_TXN_LOCKED_KEY,
  RALEIGHSL_ERRNO_TXN_LOCKED_OPERATION,

  /* Semantic related */
  RALEIGHSL_ERRNO_OBJECT_EXISTS,
  RALEIGHSL_ERRNO_OBJECT_NOT_FOUND,

  /* Object related */
  RALEIGHSL_ERRNO_OBJECT_WRONG_TYPE,

  /* Object Data related */
  RALEIGHSL_ERRNO_DATA_CAS,
  RALEIGHSL_ERRNO_DATA_KEY_EXISTS,
  RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND,
  RALEIGHSL_ERRNO_DATA_NO_ITEMS,

  /* Number related */
  RALEIGHSL_ERRNO_NUMBER_DIVMOD_BYZERO,
  RALEIGHSL_ERRNO_NUMBER_DIVMOD_OVERFLOW,

  /* Device related */

  /* Format related */

  /* Space related */

  /* Key related */
} raleighsl_errno_t;

const char *raleighsl_errno_byte_slice (raleighsl_errno_t errno,
                                        z_byte_slice_t *slice);
const char *raleighsl_errno_string     (raleighsl_errno_t errno);

#endif /* !_RALEIGHSL_ERRNO_H_ */
