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
#include <zcl/stream.h>
#include <zcl/chunkq.h>

#include "messageq_p.h"

/* ============================================================================
 *  PRIVATE - Message Queue (Thread-Safe)
 */
void z_message_queue_open (z_message_queue_t *queue) {
    z_spin_init(&(queue->lock));
    queue->head = NULL;
    queue->tail = NULL;
}

void z_message_queue_close (z_message_queue_t *queue) {
    z_spin_destroy(&(queue->lock));
}

void z_message_queue_push (z_message_queue_t *queue, z_message_t *message) {
    z_message_t *tail;

    message->next = NULL;

    z_spin_lock(&(queue->lock));
    if ((tail = queue->tail) != NULL) {
        tail->next = message;
        queue->tail = message;
    } else {
        queue->head = message;
        queue->tail = message;
    }
    z_spin_unlock(&(queue->lock));
}

z_message_t *z_message_queue_pop (z_message_queue_t *queue) {
    z_message_t *msg;

    z_spin_lock(&(queue->lock));
    if ((msg = queue->head) != NULL && (queue->head = msg->next) == NULL)
        queue->tail = NULL;
    z_spin_unlock(&(queue->lock));

    return(msg);
}

/* ============================================================================
 *  MessageQ
 */
z_messageq_t *z_messageq_alloc (z_messageq_t *messageq,
                                z_memory_t *memory,
                                z_messageq_plug_t *plug,
                                z_message_func_t exec_func,
                                void *user_data)
{
    if ((messageq = z_object_alloc(memory, messageq, z_messageq_t)) == NULL)
        return(NULL);

    messageq->exec_func = exec_func;
    messageq->user_data = user_data;
    messageq->plug = plug;

    if (plug->init(messageq)) {
        z_object_free(messageq);
        return(NULL);
    }

    return(messageq);
}

void z_messageq_free (z_messageq_t *messageq) {
    messageq->plug->uninit(messageq);
    z_object_free(messageq);
}

/* ===========================================================================
 *  Message Source
 */
z_message_source_t *z_message_source_alloc (z_messageq_t *messageq) {
    z_message_source_t *source;

    if ((source = z_object_struct_alloc(messageq, z_message_source_t)) == NULL)
        return(NULL);

    source->messageq = messageq;

    return(source);
}

void z_message_source_free (z_message_source_t *source) {
    z_object_struct_free(source->messageq, z_message_source_t, source);
}

z_messageq_t *z_message_source_queue (z_message_source_t *source) {
    return(source->messageq);
}

/* ============================================================================
 *  Message
 */
z_message_t *z_message_alloc (z_message_source_t *source, unsigned int flags) {
    z_message_t *msg;

    if ((msg = z_object_struct_alloc(source->messageq, z_message_t)) == NULL)
        return(NULL);

    msg->source   = source;
    msg->next     = NULL;

    msg->callback = NULL;
    msg->data     = NULL;

    msg->i_func   = NULL;
    msg->i_data   = NULL;

    msg->request  = NULL;
    msg->response = NULL;

    msg->flags    = flags;
    msg->state    = 0;
    msg->type     = 0;

    return(msg);
}

void z_message_free (z_message_t *message) {
    if (message->request != NULL)
        z_chunkq_clear(message->request);

    if (message->response != NULL)
        z_chunkq_clear(message->response);

    z_object_struct_free(message->source->messageq, z_message_t, message);
}

int z_message_send (z_message_t *message,
                    const z_rdata_t *object,
                    z_message_func_t callback,
                    void *user_data)
{
    z_messageq_t *messageq = message->source->messageq;

    message->callback = callback;
    message->data = user_data;

    return(messageq->plug->send(messageq, object, message));
}

void z_message_yield (z_message_t *message) {
    z_messageq_t *messageq = message->source->messageq;
    if (messageq->plug->yield != NULL)
        messageq->plug->yield(messageq, message);
}

void z_message_yield_sub_task (z_message_t *message,
                               z_message_func_t func,
                               void *data)
{
    z_message_set_sub_task(message, func, data);
    if (func != NULL)
        z_message_yield(message);
}

void *z_message_sub_task_data (z_message_t *message) {
    return(message->i_data);
}

void z_message_set_sub_task (z_message_t *message,
                             z_message_func_t func,
                             void *data)
{
    message->i_func = func;
    message->i_data = data;
}

void z_message_unset_sub_task (z_message_t *message) {
    message->i_func = NULL;
    message->i_data = NULL;
}

const z_rdata_t *z_message_object (z_message_t *message) {
    return(message->object);
}

z_message_source_t *z_message_source (z_message_t *message) {
    return(message->source);
}

z_messageq_t *z_message_queue (z_message_t *message) {
    return(message->source->messageq);
}

unsigned int z_message_flags (z_message_t *message) {
    return(message->flags);
}

unsigned int z_message_type (z_message_t *message) {
    return(message->type);
}

void z_message_set_type (z_message_t *message, unsigned int type) {
    message->type = type;
}

unsigned int z_message_state (z_message_t *message) {
    return(message->state);
}

void z_message_set_state (z_message_t *message, unsigned int state) {
    message->state = state;
}

/* ============================================================================
 *  Message request/response stream
 */
static unsigned int __message_stream_read (z_stream_t *stream,
                                           void *buffer,
                                           unsigned int n)
{
    z_message_t *message = Z_MESSAGE(stream->data.ptr);
    z_chunkq_t *chunkq = Z_CHUNKQ(stream->plug_data.ptr);
    unsigned int rd;

    if ((rd = z_chunkq_read(chunkq, stream->offset, buffer, n)) != n)
        message->flags |= Z_MESSAGE_HAS_ERRORS;

    stream->offset += rd;
    return(rd);
}

static unsigned int __message_stream_write (z_stream_t *stream,
                                            const void *buffer,
                                            unsigned int n)
{
    z_message_t *message = Z_MESSAGE(stream->data.ptr);
    z_chunkq_t *chunkq = Z_CHUNKQ(stream->plug_data.ptr);
    unsigned int wr;

    if ((wr = z_chunkq_append(chunkq, buffer, n)) != n)
        message->flags |= Z_MESSAGE_HAS_ERRORS;

    stream->offset += wr;
    return(wr);
}

static unsigned int __message_stream_fetch (z_stream_t *stream,
                                            unsigned int length,
                                            z_iopush_t fetch_func,
                                            void *user_data)
{
    z_message_t *message = Z_MESSAGE(stream->data.ptr);
    z_chunkq_t *chunkq = Z_CHUNKQ(stream->plug_data.ptr);
    unsigned int rd;

    rd = z_chunkq_fetch(chunkq, stream->offset, length, fetch_func, user_data);
    if (rd != length)
        message->flags |= Z_MESSAGE_HAS_ERRORS;

    stream->offset += rd;

    return(rd);
}

static int __message_stream_memcmp (z_stream_t *stream,
                                    const void *mem,
                                    unsigned int mem_size)
{
    z_chunkq_t *chunkq = Z_CHUNKQ(stream->plug_data.ptr);
    return(z_chunkq_memcmp(chunkq, stream->offset, mem, mem_size));
}

static z_stream_plug_t __message_stream_plug = {
    .close  = NULL,
    .seek   = NULL,
    .read   = __message_stream_read,
    .write  = __message_stream_write,
    .fetch  = __message_stream_fetch,
    .memcmp = __message_stream_memcmp,
};

int z_message_request_stream (z_message_t *message, z_stream_t *stream) {
    if (message->request == NULL) {
        z_memory_t *memory;

        memory = z_object_memory(message->source->messageq);
        if ((message->request = z_chunkq_alloc(NULL, memory, 512)) == NULL)
            return(1);
    }

    z_stream_open(stream, &__message_stream_plug);
    stream->plug_data.ptr = message->request;
    stream->data.ptr = message;
    return(0);
}

int z_message_response_stream (z_message_t *message, z_stream_t *stream) {
    if (message->response == NULL) {
        z_memory_t *memory;

        memory = z_object_memory(message->source->messageq);
        if ((message->response = z_chunkq_alloc(NULL, memory, 512)) == NULL)
            return(1);
    }

    z_stream_open(stream, &__message_stream_plug);
    stream->plug_data.ptr = message->response;
    stream->data.ptr = message;
    return(0);
}

