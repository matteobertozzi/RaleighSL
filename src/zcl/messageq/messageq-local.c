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

#include <zcl/messageq.h>
#include <zcl/thread.h>

#include "messageq_p.h"

struct localq {
};

/* ============================================================================
 *  Dispatch
 */

/* ============================================================================
 *  Localq plugin
 */
static int __localq_init (z_messageq_t *messageq) {
    return(0);
}

static void __localq_uninit (z_messageq_t *messageq) {
}

static int __localq_send (z_messageq_t *messageq, 
                          const z_rdata_t *object,
                          z_message_t *message)
{
    return(0);
}

static void __localq_yield (z_messageq_t *messageq, 
                            z_message_t *message)
{
}

z_messageq_plug_t z_messageq_local = {
    .init   = __localq_init,
    .uninit = __localq_uninit,
    .send   = __localq_send,
    .yield  = __localq_yield,
};

