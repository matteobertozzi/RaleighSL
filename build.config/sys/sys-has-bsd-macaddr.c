#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

int main (int argc, char **argv) {
  uint8_t mac_address[6];
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
  if ((mib[5] = if_nametoindex("en0")) == 0) {
    perror("if_nametoindex error");
    return(0);
  }

  len = sizeof(buffer);
  if (sysctl(mib, 6, buffer, &len, NULL, 0) < 0) {
    perror("sysctl 2 error");
    return(0);
  }

  ifm = (struct if_msghdr *)buffer;
  sdl = (struct sockaddr_dl *)(ifm + 1);
  memcpy(mac_address, LLADDR(sdl), 6);
  return(0);
}
