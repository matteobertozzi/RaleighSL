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

#ifndef _RALEIGHFS_OBJECT_DEQUE_H_
#define _RALEIGHFS_OBJECT_DEQUE_H_

#include <raleighfs/execute.h>
#include <raleighfs/plugins.h>

typedef enum raleighfs_deque_code {
    RALEIGHFS_DEQUE_PUSH_BACK       = RALEIGHFS_INSERT | 1,
    RALEIGHFS_DEQUE_PUSH_FRONT      = RALEIGHFS_INSERT | 2,
    RALEIGHFS_DEQUE_POP_BACK        = RALEIGHFS_REMOVE | 3,
    RALEIGHFS_DEQUE_POP_FRONT       = RALEIGHFS_REMOVE | 4,
    RALEIGHFS_DEQUE_GET_BACK        = RALEIGHFS_QUERY  | 5,
    RALEIGHFS_DEQUE_GET_FRONT       = RALEIGHFS_QUERY  | 6,
    RALEIGHFS_DEQUE_RM_BACK         = RALEIGHFS_REMOVE | 7,
    RALEIGHFS_DEQUE_RM_FRONT        = RALEIGHFS_REMOVE | 8,
    RALEIGHFS_DEQUE_CLEAR           = RALEIGHFS_REMOVE | 9,
    RALEIGHFS_DEQUE_LENGTH          = RALEIGHFS_QUERY  | 10,
    RALEIGHFS_DEQUE_STATS           = RALEIGHFS_QUERY  | 11,
} raleighfs_deque_code_t;

extern const uint8_t RALEIGHFS_OBJECT_DEQUE_UUID[16];
extern raleighfs_object_plug_t raleighfs_object_deque;

#endif /* !_RALEIGHFS_OBJECT_DEQUE_H_ */

