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

#include <raleighfs/object.h>

#include "deque_p.h"
#include "deque.h"

static void __dobject_free (void *user_data, void *ptr) {
    deque_object_free(Z_MEMORY(user_data), DEQUE_OBJECT(ptr));
}

deque_t *deque_alloc (z_memory_t *memory) {
    deque_t *deque;

    if ((deque = z_memory_struct_alloc(memory, deque_t)) == NULL)
        return(NULL);

    if (!z_deque_alloc(&(deque->deque), memory, __dobject_free, memory)) {
        z_memory_struct_free(memory, deque_t, deque);
        return(NULL);
    }

    deque->length = 0;
    deque->size = 0;

    return(deque);
}

void deque_free (z_memory_t *memory, deque_t *deque) {
    z_deque_free(&(deque->deque));
    z_memory_struct_free(memory, deque_t, deque);
}

void deque_clear (deque_t *deque) {
    deque->size = 0;
    deque->length = 0;
    z_deque_clear(&(deque->deque));
}

raleighfs_errno_t deque_push_back (deque_t *deque,
                                   const z_stream_extent_t *value)
{
    deque_object_t *dobject;
    z_memory_t *memory;

    memory = z_object_memory(&(deque->deque));
    if ((dobject = deque_object_alloc(memory, value)) == NULL)
        return(RALEIGHFS_ERRNO_NO_MEMORY);

    if (z_deque_push_back(&(deque->deque), dobject)) {
        deque_object_free(memory, dobject);
        return(RALEIGHFS_ERRNO_NO_MEMORY);
    }

    deque->length++;
    deque->size += value->length;
    return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t deque_push_front (deque_t *deque,
                                    const z_stream_extent_t *value)
{
    deque_object_t *dobject;
    z_memory_t *memory;

    memory = z_object_memory(&(deque->deque));
    if ((dobject = deque_object_alloc(memory, value)) == NULL)
        return(RALEIGHFS_ERRNO_NO_MEMORY);

    if (z_deque_push_front(&(deque->deque), dobject)) {
        deque_object_free(memory, dobject);
        return(RALEIGHFS_ERRNO_NO_MEMORY);
    }

    deque->length++;
    deque->size += value->length;
    return(RALEIGHFS_ERRNO_NONE);
}

deque_object_t *deque_pop_front (deque_t *deque) {
    deque_object_t *object;

    if ((object = DEQUE_OBJECT(z_deque_pop_front(&(deque->deque)))) != NULL) {
        deque->size -= object->length;
        deque->length--;
    }

    return(object);
}

deque_object_t *deque_pop_back (deque_t *deque) {
    deque_object_t *object;

    if ((object = DEQUE_OBJECT(z_deque_pop_back(&(deque->deque)))) != NULL) {
        deque->size -= object->length;
        deque->length--;
    }

    return(object);
}

deque_object_t *deque_get_front (deque_t *deque) {
    return(DEQUE_OBJECT(z_deque_peek_front(&(deque->deque))));
}

deque_object_t *deque_get_back (deque_t *deque) {
    return(DEQUE_OBJECT(z_deque_peek_back(&(deque->deque))));
}

int deque_remove_front (deque_t *deque) {
    deque_object_t *dobject;

    if ((dobject = deque_pop_front(deque)) != NULL)
        deque_object_free(z_object_memory(&(deque->deque)), dobject);

    return(dobject == NULL);
}

int deque_remove_back (deque_t *deque) {
    deque_object_t *dobject;

    if ((dobject = deque_pop_back(deque)) != NULL)
        deque_object_free(z_object_memory(&(deque->deque)), dobject);

    return(dobject == NULL);
}


deque_object_t *deque_object_alloc (z_memory_t *memory,
                                    const z_stream_extent_t *value)
{
    deque_object_t *obj;
    unsigned int size;

    size = sizeof(deque_object_t) + (value->length - 1);
    if ((obj = z_memory_block_alloc(memory, deque_object_t, size)) != NULL) {
        obj->length = value->length;
        z_stream_read_extent(value, obj->data);
    }

    return(obj);
}

void deque_object_free (z_memory_t *memory,
                        deque_object_t *dobject)
{
    z_memory_block_free(memory, dobject);
}


