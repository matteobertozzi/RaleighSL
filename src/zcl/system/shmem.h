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

#ifndef _Z_SHMEM_H_
#define _Z_SHMEM_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_shmem {
  uint8_t *   addr;
  uint64_t    size;
  const char *path;
} z_shmem_t;

int z_shmem_create  (z_shmem_t *self, const char *path, uint64_t size);
int z_shmem_open    (z_shmem_t *self, const char *path, int rdonly);
int z_shmem_close   (z_shmem_t *self, int destroy);

__Z_END_DECLS__

#endif /* !_Z_SHMEM_H_ */
