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

#include "messageq_p.h"

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

    if (messageq->plug->init(messageq)) {
        z_object_free(messageq);
        return(NULL);
    }

    return(messageq);
}

void z_messageq_free (z_messageq_t *messageq) {
    messageq->plug->uninit(messageq);
    z_object_free(messageq);
}

z_message_t *z_message_alloc (z_messageq_t *messageq,
                              z_message_source_t *source,
                              unsigned int flags)
{
    z_message_t *message;

    if ((message = z_object_struct_alloc(messageq, z_message_t)) == NULL)
        return(NULL);


    message->messageq = messageq;
    message->source = source;
    message->flags = flags;
    message->state = 0;
    message->type = 0;

    if (!z_chunkq_alloc(&(message->request), z_object_memory(messageq), 512)) {
        z_object_struct_free(messageq, z_message_t, message);
        return(NULL);
    }

    if (!z_chunkq_alloc(&(message->response), z_object_memory(messageq), 512)) {
        z_chunkq_free(&(message->response));
        z_object_struct_free(messageq, z_message_t, message);
        return(NULL);
    }

    return(message);
}

void z_message_free (z_message_t *message) {
    z_chunkq_free(&(message->request));
    z_chunkq_free(&(message->response));
    z_object_struct_free(message->messageq, z_message_t, message);
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

unsigned int z_message_flags (z_message_t *message) {
    return(message->flags);
}

z_messageq_t *z_message_queue (z_message_t *message) {
    return(message->messageq);
}

int z_message_send (z_message_t *message,
                    const void *obj_name,
                    unsigned int obj_nlength,
                    z_message_func_t callback,
                    void *udata)
{
    z_messageq_t *q = message->messageq;

    if (message->flags & Z_MESSAGE_HAS_ERRORS)
        return(-1);

    message->flags &= ~Z_MESSAGE_HAS_ERRORS;
    return(q->plug->send(q, message, obj_name, obj_nlength, callback, udata));
}



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

static z_stream_plug_t __message_stream_plug = {
    .close  = NULL,
    .seek   = NULL,
    .read   = __message_stream_read,
    .write  = __message_stream_write,
};

int z_message_request_stream (z_message_t *message, z_stream_t *stream) {
    z_stream_open(stream, &__message_stream_plug);
    stream->plug_data.ptr = &(message->request);
    stream->data.ptr = message;
    return(0);
}

int z_message_response_stream (z_message_t *message, z_stream_t *stream) {
    z_stream_open(stream, &__message_stream_plug);
    stream->plug_data.ptr = &(message->response);
    stream->data.ptr = message;
    return(0);
}

