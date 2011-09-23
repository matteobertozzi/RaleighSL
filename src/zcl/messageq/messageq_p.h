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

#ifndef _Z_MESSAGEQ_PRIVATE_H_
#define _Z_MESSAGEQ_PRIVATE_H_

#include <zcl/messageq.h>
#include <zcl/chunkq.h>

#include <zcl/thread.h>

struct z_message_source {
    z_messageq_t *messageq;
    z_mutex_t lock;

    /* pending */
    void *pending_head;
    void *pending_tail;

    /* ordered */
    void *ordered_head;
    void *ordered_tail;

    /* unordered */
    void *unordered_head;
    void *unordered_tail;
};

struct z_message {
    z_message_source_t *source;
    z_messageq_t *      messageq;

    unsigned int        flags;
    unsigned int        state;
    unsigned int        type;

    z_chunkq_t          request;
    z_chunkq_t          response;
};

#endif /* !_Z_MESSAGEQ_PRIVATE_H_ */
