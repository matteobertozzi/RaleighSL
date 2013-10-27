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

#ifndef _Z_DATA_H_
#define _Z_DATA_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/buffer.h>
#include <zcl/bytes.h>

Z_TYPEDEF_ENUM(z_data_type)
Z_TYPEDEF_ENUM(z_data_order)
Z_TYPEDEF_UNION(z_data_value)

enum z_data_order {
  Z_ORDER_ASCENDING,
  Z_ORDER_DESCENDING,
};

enum z_data_type {
  Z_TYPE_BOOLEAN,
  Z_TYPE_INTEGER,
  Z_TYPE_FLOATING,
  Z_TYPE_TIMESTAMP,
  Z_TYPE_BLOB,
};

union z_data_value {
  int       boolean;
  int64_t   integer;
  double    floating;
  uint64_t  timestamp;
  z_bytes_t *blob;
};

void  z_data_value_free     (z_data_type_t type,
                             const z_data_value_t *value);

void  z_data_value_set      (z_data_type_t type,
                             z_data_value_t *value,
                             const z_buffer_t *bytes);

int   z_data_value_compare  (z_data_type_t type,
                             z_data_order_t order,
                             const z_data_value_t *a,
                             const z_data_value_t *b);

__Z_END_DECLS__

#endif /* _Z_DATA_H_ */
