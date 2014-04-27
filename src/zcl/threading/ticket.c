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

#include <zcl/atomic.h>
#include <zcl/ticket.h>
#include <zcl/system.h>
#if 0
void z_ticket_init (z_ticket_t *ticket) {
  ticket->s.now_serving = 0;
  ticket->s.next_ticket = 0;
}

void z_ticket_acquire (z_ticket_t *ticket) {
  uint16_t my_ticket;
  my_ticket = z_atomic_fetch_and_add(&(ticket->s.next_ticket), 1);
  while (ticket->s.now_serving != my_ticket) {
    z_system_cpu_relax();
  }
}

void z_ticket_release (z_ticket_t *ticket) {
  z_atomic_synchronize();
  ticket->s.now_serving += 1;
}
#endif