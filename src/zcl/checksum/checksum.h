/*
 *   Copyright 2011 Matteo Bertozzi
 *
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

#ifndef _Z_CHECKSUM_H_
#define _Z_CHECKSUM_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/types.h>

typedef uint16_t (*z_csum16_t)    (void *user_data,
                                   const void *data,
                                   unsigned int n);
typedef uint32_t (*z_csum32_t)    (void *user_data,
                                   const void *data,
                                   unsigned int n);


uint16_t    z_csum16_crc           (uint16_t csum,
                                    const void *data,
                                    unsigned int n);

uint32_t    z_csum32_adler         (uint32_t csum,
                                    const void *data,
                                    unsigned int n);

uint32_t    z_csum32_crcc          (uint32_t csum,
                                    const void *data,
                                    unsigned int n);

__Z_END_DECLS__

#endif /* !_Z_CHECKSUM_H_ */

