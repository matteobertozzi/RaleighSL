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

#include "private.h"

#if 0
    #define __LOG(...)              fprintf(stderr, __VA_ARGS__)
#else
    #define __LOG(...)
#endif

void __observer_notify_create (raleighfs_t *fs,
                               const z_rdata_t *name)
{
    __LOG("__observer_notify_create()\n");
}

void __observer_notify_open (raleighfs_t *fs,
                             const z_rdata_t *name)
{
    __LOG("__observer_notify_open()\n");
}

void __observer_notify_rename (raleighfs_t *fs,
                               const z_rdata_t *old_name,
                               const z_rdata_t *new_name)
{
    __LOG("__observer_notify_rename()\n");
}

void __observer_notify_unlink (raleighfs_t *fs,
                               const z_rdata_t *name)
{
    __LOG("__observer_notify_unlink()\n");
}


void __observer_notify_insert (raleighfs_t *fs,
                               raleighfs_object_t *object,
                               z_message_t *message)
{
    __LOG("__observer_notify_insert()\n");
}

void __observer_notify_update (raleighfs_t *fs,
                               raleighfs_object_t *object,
                               z_message_t *message)
{
    __LOG("__observer_notify_update()\n");
}

void __observer_notify_remove (raleighfs_t *fs,
                               raleighfs_object_t *object,
                               z_message_t *message)
{
    __LOG("__observer_notify_remove()\n");
}

void __observer_notify_ioctl (raleighfs_t *fs,
                              raleighfs_object_t *object,
                              z_message_t *message)
{
    __LOG("__observer_notify_ioctl()\n");
}

