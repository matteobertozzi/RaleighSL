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
#include <zcl/thread.h>
#include <zcl/chunkq.h>

Z_TYPEDEF_STRUCT(z_message_queue)

struct z_message_source {
    z_messageq_t *messageq;
};

struct z_message {
    z_message_source_t *source;          /* Source of the message */
    z_message_t *       next;            /* Next message in the messageq */

    z_message_func_t    callback;        /* User callback function */
    void *              data;            /* User data associated with the msg */

    z_rdata_t *         object;          /* Destination object id */

    z_message_func_t    i_func;          /* Internal function */
    void *              i_data;          /* Internal data */

    z_chunkq_t          *request;        /* Request data */
    z_chunkq_t          *response;       /* Response data */

    unsigned int        flags;           /* Message flags */
    unsigned int        state;           /* Message state */
    unsigned int        type;            /* Message type */
};

struct z_message_queue {
    z_message_t *head;
    z_message_t *tail;
    z_spinlock_t lock;
};

void            z_message_queue_open    (z_message_queue_t *queue);
void            z_message_queue_close   (z_message_queue_t *queue);
void            z_message_queue_push    (z_message_queue_t *queue,
                                         z_message_t *message);
z_message_t *   z_message_queue_pop     (z_message_queue_t *queue);

#endif /* !_Z_MESSAGEQ_PRIVATE_H_ */

