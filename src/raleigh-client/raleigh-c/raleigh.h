/*
 *   Copyright 2007-2013 Matteo Bertozzi
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

#ifndef _RALEIGH_CLIENT_H_
#define _RALEIGH_CLIENT_H_

#define RALEIGH_CLIENT(x)           Z_CAST(raleigh_client_t, x)

typedef struct raleigh_client raleigh_client_t;

int  raleigh_initialize   (void);
void raleigh_uninitialize (void);

raleigh_client_t *raleigh_tcp_connect (const char *address, const char *port, int async);

typedef void (*raleigh_ping_t) (void *udata);
int raleigh_ping_async (raleigh_client_t *self, raleigh_ping_t callback, void *udata);
int raleigh_ping       (raleigh_client_t *self);

#endif /* !_RALEIGH_CLIENT_H_ */

