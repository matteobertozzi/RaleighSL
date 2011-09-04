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

#ifndef _RALEIGHFS_OBJECT_COUNTER_H_
#define _RALEIGHFS_OBJECT_COUNTER_H_

#include <raleighfs/execute.h>
#include <raleighfs/plugins.h>

typedef enum raleighfs_counter_code {
    RALEIGHFS_COUNTER_GET       = RALEIGHFS_QUERY  | 1,
    RALEIGHFS_COUNTER_SET       = RALEIGHFS_UPDATE | 2,
    RALEIGHFS_COUNTER_CAS       = RALEIGHFS_UPDATE | 3,
    RALEIGHFS_COUNTER_INCR      = RALEIGHFS_UPDATE | 4,
    RALEIGHFS_COUNTER_DECR      = RALEIGHFS_UPDATE | 5,
} raleighfs_counter_code_t;

extern const uint8_t RALEIGHFS_OBJECT_COUNTER_UUID[16];
extern raleighfs_object_plug_t raleighfs_object_counter;

#endif /* !_RALEIGHFS_OBJECT_COUNTER_H_ */

