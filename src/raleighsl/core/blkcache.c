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

#include <raleighsl/errno.h>

#include "private.h"

/*
 * +--------------------------+
 * | cache entry   (40 bytes) |
 * +--------------------------+
 * |  - hash-next  ( 8 bytes) |
 * |  - cnode-link (16 bytes) |
 * |  - OID        ( 8 bytes) |
 * |  - refs       ( 4 bytes) |
 * |  - state      ( 4 bytes) |
 * +--------------------------+
 * | block         (72 bytes) |
 * +--------------------------+
 * |  - block hash (32 bytes) |
 * |  - hash node  (24 bytes) |
 * |  - length     ( 4 bytes) |
 * |  - pad        ( 4 bytes) |
 * |  - data-ptr   ( 8 bytes) |
 * +--------------------------+
 * |              (112 bytes) |
 * +--------------------------+
 */

struct blk_page {
  struct blk_group *next;
  unsigned int x;
  unsigned int y;
  raleighsl_block_t block[73];
};
