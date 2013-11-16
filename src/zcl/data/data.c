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

#include <zcl/data.h>

void z_data_value_free (z_data_type_t type, const z_data_value_t *value) {
  if (type == Z_TYPE_BLOB) {
    z_bytes_free(value->blob);
  }
}

void z_data_value_set (z_data_type_t type, z_data_value_t *value, const z_buffer_t *bytes) {
}

int z_data_value_compare (z_data_type_t type, z_data_order_t order,
                          const z_data_value_t *a, const z_data_value_t *b)
{
  int cmp;
  switch (type) {
    case Z_TYPE_BOOLEAN:
      cmp = z_cmp(a->boolean, b->boolean);
      break;
    case Z_TYPE_INTEGER:
      cmp = z_cmp(a->integer, b->integer);
      break;
    case Z_TYPE_FLOATING:
      cmp = z_cmp(a->floating, b->floating);
      break;
    case Z_TYPE_TIMESTAMP:
      cmp = z_cmp(a->timestamp, b->timestamp);
      break;
    case Z_TYPE_BLOB:
      cmp = z_bytes_compare(a->blob, b->blob);
      break;
    default:
      cmp = 0;
      break;
  }
  return(order == Z_ORDER_ASCENDING ? cmp : -cmp);
}