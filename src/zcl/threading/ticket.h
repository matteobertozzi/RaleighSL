/*
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

#ifndef _Z_TICKET_H_
#define _Z_TICKET_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/atomic.h>
#include <zcl/system.h>

#define Z_TICKET_INLINE     1

typedef struct z_ticket {
  uint16_t next_ticket;
  uint16_t now_serving;
} z_ticket_t;

#if Z_TICKET_INLINE
#define z_ticket_init(self)           \
  do {                                \
    (self)->now_serving = 0;          \
    (self)->next_ticket = 0;          \
  } while (0)

#define z_ticket_acquire(self)                                          \
  do {                                                                  \
    uint16_t my_ticket;                                                 \
    my_ticket = z_atomic_fetch_and_add(&((self)->next_ticket), 1);      \
    while ((self)->now_serving != my_ticket) {                          \
      z_system_cpu_relax();                                             \
    }                                                                   \
  } while (0)

#define z_ticket_release(self)          \
  do {                                  \
    z_atomic_synchronize();             \
    (self)->now_serving += 1;           \
  } while (0)
#else
void  z_ticket_init      (z_ticket_t *ticket);
void  z_ticket_acquire   (z_ticket_t *ticket);
void  z_ticket_release   (z_ticket_t *ticket);
#endif

__Z_END_DECLS__

#endif /* !_Z_TICKET_H_ */