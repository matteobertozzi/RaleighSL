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

#ifndef _Z_SBUFFER_H_
#define _Z_SBUFFER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memslice.h>
#include <zcl/macros.h>
#include <zcl/data.h>

void  z_sbuffer_init    (uint8_t *sbuffer,
                         uint16_t size,
                         z_data_type_t key_type,
                         uint16_t ksize,
                         uint16_t vsize);
void  z_sbuffer_add     (uint8_t *sbuffer,
                         const z_memslice_t *key,
                         const z_memslice_t *value);
int   z_sbuffer_remove  (uint8_t *sbuffer,
                         const z_memslice_t *key);
int   z_sbuffer_get     (const uint8_t *sbuffer,
                         unsigned int index,
                         z_memslice_t *key,
                         z_memslice_t *value);
int   z_sbuffer_lookup  (const uint8_t *sbuffer,
                         const z_memslice_t *key,
                         z_memslice_t *value);
void  z_sbuffer_debug   (const uint8_t *sbuffer,
                         FILE *stream);

__Z_END_DECLS__

#endif /* !_Z_SBUFFER_H_ */
