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

#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>

int main (int argc, char **argv) {
  struct ifaddrs *ifaddr, *ifa;
  uint8_t mac_address[6];
  struct ifconf ifc;
  struct ifreq ifr;
  char buf[1024];
  int sock;

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
    perror("socket(IPPROTO_IP)");
    return(1);
  };

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return(1);
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
      perror("ioctl(SIOCGIFCONF)");
      return(2);
    }

    strcpy(ifr.ifr_name, ifa->ifa_name);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
      perror("ioctl(SIOCGIFFLAGS)");
      return(3);
    }
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
      perror("ioctl(SIOCGIFHWADDR)");
      return(4);
    }

    if (getifaddrs(&ifaddr) == -1) {
      perror("getifaddrs");
      return(1);
    }

    memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
    printf("%-8s %02x:%02x:%02x:%02x:%02x:%02x\n",
           ifa->ifa_name,
           mac_address[0], mac_address[1], mac_address[2],
           mac_address[3], mac_address[4], mac_address[5]);
  }

  freeifaddrs(ifaddr);
  return(0);
}
