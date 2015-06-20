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

#include <zcl/config.h>
#include <zcl/debug.h>

#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>

#if defined(Z_SYS_HAS_BSD_MACADDR)
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/sysctl.h>
  #include <net/if_dl.h>
  #include <arpa/inet.h>
#endif

int z_mac_address (uint8_t mac_address[6], const char *iface) {
#if defined(Z_SYS_HAS_LINUX_MACADDR)
  struct ifconf ifc;
  struct ifreq ifr;
  char buf[1024];
  int sock;

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
    perror("socket(IPPROTO_IP)");
    return(1);
  };

  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
    perror("ioctl(SIOCGIFCONF)");
    return(2);
  }

  strcpy(ifr.ifr_name, iface);
  if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
    perror("ioctl(SIOCGIFFLAGS)");
    return(3);
  }
  if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
    perror("ioctl(SIOCGIFHWADDR)");
    return(4);
  }

  memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
  return(0);
#elif defined(Z_SYS_HAS_BSD_MACADDR)
  const struct sockaddr_dl *sdl;
  const struct if_msghdr *ifm;
  uint8_t buffer[1024];
  int mib[6];
  size_t len;

  mib[0] = CTL_NET;
  mib[1] = AF_ROUTE;
  mib[2] = 0;
  mib[3] = AF_LINK;
  mib[4] = NET_RT_IFLIST;
  if ((mib[5] = if_nametoindex(iface)) == 0) {
    perror("if_nametoindex error");
    return(1);
  }

  len = sizeof(buffer);
  if (sysctl(mib, 6, buffer, &len, NULL, 0) < 0) {
    perror("sysctl 2 error");
    return(2);
  }

  ifm = (struct if_msghdr *)buffer;
  sdl = (struct sockaddr_dl *)(ifm + 1);
  memcpy(mac_address, LLADDR(sdl), 6);
  return(0);
#endif /* Z_SYS_HAS_BSD_MACADDR */
  return(-1);
}
