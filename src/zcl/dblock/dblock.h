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

#ifndef _Z_DBLOCK_H_
#define _Z_DBLOCK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef enum z_dblock_overlap z_dblock_overlap_t;
typedef enum z_dblock_seek z_dblock_seek_t;

enum z_dblock_seek {
  Z_DBLOCK_SEEK_BEGIN,
  Z_DBLOCK_SEEK_END,
  Z_DBLOCK_SEEK_LT,
  Z_DBLOCK_SEEK_LE,
  Z_DBLOCK_SEEK_GT,
  Z_DBLOCK_SEEK_GE,
  Z_DBLOCK_SEEK_EQ,
};

enum z_dblock_overlap {
  Z_DBLOCK_OVERLAP_YES,
  Z_DBLOCK_OVERLAP_YES_LEFT,
  Z_DBLOCK_OVERLAP_YES_RIGHT,
  Z_DBLOCK_OVERLAP_NO_LEFT,
  Z_DBLOCK_OVERLAP_NO_RIGHT,
};

__Z_END_DECLS__

#endif /* !_Z_DBLOCK_H_ */
