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

#ifndef _Z_RWCSEM_H_
#define _Z_RWCSEM_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define Z_RWCSEM_WRITE_FLAG         (1 << 29)    /* 0b00100000000000000000000000000000 */
#define Z_RWCSEM_COMMIT_FLAG        (1 << 30)    /* 0b01000000000000000000000000000000 */
#define Z_RWCSEM_LOCK_FLAG          (1 << 31)    /* 0b10000000000000000000000000000000 */
#define Z_RWCSEM_READERS_MASK       (0x1fffffff) /* 0b00011111111111111111111111111111 */
#define Z_RWCSEM_RW_MASK            (0x3fffffff) /* 0b00111111111111111111111111111111 */
#define Z_RWCSEM_CW_MASK            (0x60000000) /* 0b01100000000000000000000000000000 */
#define Z_RWCSEM_LC_MASK            (0xC0000000) /* 0b11000000000000000000000000000000 */

typedef struct z_rwcsem {
  uint32_t state;
} z_rwcsem_t;

typedef enum z_rwcsem_op {
  Z_RWCSEM_READ,
  Z_RWCSEM_WRITE,
  Z_RWCSEM_COMMIT,
  Z_RWCSEM_LOCK,
} z_rwcsem_op_t;

#define z_rwcsem_is_readable(state)     (((state) & Z_RWCSEM_RW_MASK) == (state))
#define z_rwcsem_is_writable(state)     (((state) & Z_RWCSEM_READERS_MASK) == (state))
#define z_rwcsem_is_committable(state)  (((state) & Z_RWCSEM_COMMIT_FLAG) == (state))
#define z_rwcsem_is_lockable(state)     (((state) & Z_RWCSEM_LOCK_FLAG) == (state))

int   z_rwcsem_init                 (z_rwcsem_t *lock);

int   z_rwcsem_try_acquire_read     (z_rwcsem_t *lock);
int   z_rwcsem_release_read         (z_rwcsem_t *lock);

int   z_rwcsem_try_acquire_write    (z_rwcsem_t *lock);
int   z_rwcsem_release_write        (z_rwcsem_t *lock);

int   z_rwcsem_has_commit_flag      (z_rwcsem_t *lock);
void  z_rwcsem_set_commit_flag      (z_rwcsem_t *lock);
int   z_rwcsem_try_acquire_commit   (z_rwcsem_t *lock);
int   z_rwcsem_release_commit       (z_rwcsem_t *lock);

int   z_rwcsem_has_lock_flag        (z_rwcsem_t *lock);
void  z_rwcsem_set_lock_flag        (z_rwcsem_t *lock);
int   z_rwcsem_try_acquire_lock     (z_rwcsem_t *lock);
int   z_rwcsem_release_lock         (z_rwcsem_t *lock);

int   z_rwcsem_try_acquire  (z_rwcsem_t *lock, z_rwcsem_op_t operation);
int   z_rwcsem_release      (z_rwcsem_t *lock, z_rwcsem_op_t operation);

int   z_rwcsem_try_switch   (z_rwcsem_t *lock,
                             z_rwcsem_op_t op_current,
                             z_rwcsem_op_t op_next);

__Z_END_DECLS__

#endif /* !_Z_RWCSEM_H_ */
