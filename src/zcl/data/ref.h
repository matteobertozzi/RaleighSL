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

#ifndef _Z_REF_H_
#define _Z_REF_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_ref_vtable z_ref_vtable_t;
typedef struct z_ref_data z_ref_data_t;

struct z_ref_vtable {
  void (*inc_ref) (void *udata, void *object);
  void (*dec_ref) (void *udata, void *object);
};

struct z_ref_data {
  uint32_t offset;
  uint32_t length;
  uint8_t *data;
};

__Z_END_DECLS__

#endif /* !_Z_REF_H_ */
