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

#include <stdio.h>

#include <zcl/messageq.h>
#include <zcl/thread.h>

#include "messageq_p.h"

#define __NOOPQ_USE_LOCK                    1

#define __NOOPQ_LOCK_INIT(noopq)            z_mutex_init(&((noopq)->lock))
#define __NOOPQ_LOCK_DESTROY(noopq)         z_mutex_destroy(&((noopq)->lock))
#define __NOOPQ_LOCK(noopq)                 z_mutex_lock(&((noopq)->lock))
#define __NOOPQ_UNLOCK(noopq)               z_mutex_unlock(&((noopq)->lock))

#define __NOOPQ(x)                          Z_CAST(struct noopq, x)

#if __NOOPQ_USE_LOCK
struct noopq {
    z_mutex_t lock;
};
#endif /* __NOOPQ_USE_LOCK */

/* ============================================================================
 *  Noopq plugin
 */
static int __noopq_init (z_messageq_t *messageq) {
#if __NOOPQ_USE_LOCK
    struct noopq *noopq;

    if ((noopq = z_object_struct_alloc(messageq, struct noopq)) == NULL)
        return(-1);

    if (__NOOPQ_LOCK_INIT(noopq)) {
        z_object_struct_free(messageq, struct noopq, noopq);
        return(-2);
    }

    messageq->plug_data.ptr = noopq;
#endif /* __NOOPQ_USE_LOCK */
    return(0);
}

static void __noopq_uninit (z_messageq_t *messageq) {
#if __NOOPQ_USE_LOCK
    struct noopq *noopq = __NOOPQ(messageq->plug_data.ptr);
    __NOOPQ_LOCK_DESTROY(noopq);
    z_object_struct_free(messageq, struct noopq, noopq);
#endif /* __NOOPQ_USE_LOCK */
}

static int __noopq_send (z_messageq_t *messageq, 
                         const z_rdata_t *object,
                         z_message_t *message)
{
#if __NOOPQ_USE_LOCK
    struct noopq *noopq = __NOOPQ(messageq->plug_data.ptr);

    __NOOPQ_LOCK(noopq);
#endif /* __NOOPQ_USE_LOCK */

    /* object doesn't need to be copied with noop queue */
    message->object = (z_rdata_t *)object;
    
    if (!z_message_is_bypass(message)) {
        if (messageq->exec_func != NULL)
            messageq->exec_func(messageq->user_data, message);

        while (message->i_func != NULL)
            message->i_func(messageq->user_data, message);
    }

    if (message->callback != NULL)
        message->callback(message->data, message);

    /* unset object for the next free */
    message->object = NULL;

#if __NOOPQ_USE_LOCK
    __NOOPQ_UNLOCK(noopq);
#endif /* __NOOPQ_USE_LOCK */
    return(0);
}

z_messageq_plug_t z_messageq_noop = {
    .init   = __noopq_init,
    .uninit = __noopq_uninit,
    .send   = __noopq_send,
    .yield  = NULL,
};

