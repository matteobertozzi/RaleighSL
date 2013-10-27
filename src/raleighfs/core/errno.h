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

#ifndef _RALEIGHFS_ERRNO_H_
#define _RALEIGHFS_ERRNO_H_

#include <zcl/byteslice.h>

typedef enum raleighfs_errno {
  /* Info */
  RALEIGHFS_ERRNO_NONE,
  RALEIGHFS_ERRNO_NOT_IMPLEMENTED,
  RALEIGHFS_ERRNO_RETRY,

  /* System related */
  RALEIGHFS_ERRNO_NO_MEMORY,

  /* Plugins related */
  RALEIGHFS_ERRNO_PLUGIN_NOT_LOADED,

  /* Transaction related */
  RALEIGHFS_ERRNO_TXN_CLOSED,
  RALEIGHFS_ERRNO_TXN_NOT_FOUND,
  RALEIGHFS_ERRNO_TXN_ROLLEDBACK,
  RALEIGHFS_ERRNO_TXN_LOCKED_KEY,
  RALEIGHFS_ERRNO_TXN_LOCKED_OPERATION,

  /* Semantic related */
  RALEIGHFS_ERRNO_OBJECT_EXISTS,
  RALEIGHFS_ERRNO_OBJECT_NOT_FOUND,

  /* Object related */
  RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE,

  /* Object Data related */
  RALEIGHFS_ERRNO_DATA_CAS,
  RALEIGHFS_ERRNO_DATA_KEY_EXISTS,
  RALEIGHFS_ERRNO_DATA_KEY_NOT_FOUND,
  RALEIGHFS_ERRNO_DATA_NO_ITEMS,

  /* Device related */

  /* Format related */

  /* Space related */

  /* Key related */
} raleighfs_errno_t;

const char *raleighfs_errno_byte_slice (raleighfs_errno_t errno,
                                        z_byte_slice_t *slice);
const char *raleighfs_errno_string     (raleighfs_errno_t errno);

#endif /* !_RALEIGHFS_ERRNO_H_ */
