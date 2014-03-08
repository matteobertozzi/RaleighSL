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

Z_TYPEDEF_ENUM(z_data_type)

enum z_data_type {
  Z_DATA_TYPE_INT,
  Z_DATA_TYPE_UINT,
  Z_DATA_TYPE_FLOAT,
  Z_DATA_TYPE_BYTES,

  /*
   * Keep the base type in the beginning,
   * they may be used in 3/4bit by some struct like sbuffer.
   */

  Z_DATA_TYPE_BIT,
};

#define z_data_int64_from_ptr(ptr, size) ({                     \
  int64_t __value;                                              \
  switch (size) {                                               \
    case 1: __value = Z_INT8_PTR_VALUE(ptr);                    \
    case 2: __value = Z_INT16_PTR_VALUE(ptr);                   \
    case 4: __value = Z_INT32_PTR_VALUE(ptr);                   \
    case 8: __value = Z_INT64_PTR_VALUE(ptr);                   \
  }                                                             \
  __value;                                                      \
})

#define z_data_uint64_from_ptr(ptr, size) ({                    \
  uint64_t __value;                                             \
  switch (size) {                                               \
    case 1: __value = Z_UINT8_PTR_VALUE(ptr);                   \
    case 2: __value = Z_UINT16_PTR_VALUE(ptr);                  \
    case 4: __value = Z_UINT32_PTR_VALUE(ptr);                  \
    case 8: __value = Z_UINT64_PTR_VALUE(ptr);                  \
  }                                                             \
  __value;                                                      \
})

#define z_data_float_from_ptr(ptr, size) ({                    \
  double __value;                                              \
  switch (size) {                                              \
    case 4: __value = Z_FLOAT_PTR_VALUE(ptr);                  \
    case 8: __value = Z_DOUBLE_PTR_VALUE(ptr);                 \
  }                                                            \
  __value;                                                     \
})

#define z_data_int_cmp(aptr, asize, bptr, bsize) ({            \
  int64_t __a_val, __b_val;                                    \
  __a_val = z_data_int64_from_ptr(aptr, asize);                \
  __b_val = z_data_int64_from_ptr(bptr, bsize);                \
  z_cmp(__a_val, __b_val);                                     \
})

#define z_data_uint_cmp(aptr, asize, bptr, bsize) ({           \
  uint64_t __a_val, __b_val;                                   \
  __a_val = z_data_uint64_from_ptr(aptr, asize);               \
  __b_val = z_data_uint64_from_ptr(bptr, bsize);               \
  z_cmp(__a_val, __b_val);                                     \
})

#define z_data_float_cmp(aptr, asize, bptr, bsize) ({          \
  double __a_val, __b_val;                                     \
  __a_val = z_data_float_from_ptr(aptr, asize);                \
  __b_val = z_data_float_from_ptr(bptr, bsize);                \
  z_cmp(__a_val, __b_val);                                     \
})

#define z_data_cmp(data_type, aptr, asize, bptr, bsize) ({                \
  int __cmp_res;                                                          \
  switch (data_type) {                                                    \
    case Z_DATA_TYPE_UINT:                                                \
      __cmp_res = z_data_uint_cmp(aptr, asize, bptr, bsize);              \
      break;                                                              \
    case Z_DATA_TYPE_INT:                                                 \
      __cmp_res = z_data_int_cmp(aptr, asize, bptr, bsize);               \
      break;                                                              \
    case Z_DATA_TYPE_FLOAT:                                               \
      __cmp_res = z_data_float_cmp(aptr, asize, bptr, bsize);             \
      break;                                                              \
    default:                                                              \
      __cmp_res = z_mem_compare(aptr, asize, bptr, bsize);                \
      break;                                                              \
  }                                                                       \
  __cmp_res;                                                              \
})

__Z_END_DECLS__

#endif /* _Z_DATA_H_ */
