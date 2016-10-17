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

#ifndef _Z_IFADDR_H_
#define _Z_IFADDR_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#if defined(Z_SYS_HAS_IFADDRS)
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <ifaddrs.h>
#endif

#include <netdb.h>

typedef struct z_ifaddr_scanner {
#if defined(Z_SYS_HAS_IFADDRS)
  struct ifaddrs *ifaddr;
  struct ifaddrs *ifa;
#endif
  char host[NI_MAXHOST];
  const char *iface;
  int family;
} z_ifaddr_scanner_t;

int   z_ifaddr_scanner_open   (z_ifaddr_scanner_t *self);
void  z_ifaddr_scanner_close  (z_ifaddr_scanner_t *self);

int   z_ifaddr_scanner_get    (z_ifaddr_scanner_t *self,
                               int *family,
                               char host[NI_MAXHOST]);
int   z_ifaddr_scanner_next   (z_ifaddr_scanner_t *self);

int   z_mac_address           (uint8_t mac_address[6], const char *iface);

__Z_END_DECLS__

#endif /* !_Z_IFADDR_H_ */
