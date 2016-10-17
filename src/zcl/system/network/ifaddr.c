#include <zcl/ifaddr.h>

#if defined(Z_SYS_HAS_IFADDRS)
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <ifaddrs.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
#endif

int z_ifaddr_scanner_open (z_ifaddr_scanner_t *self) {
#if defined(Z_SYS_HAS_IFADDRS)
  if (getifaddrs(&(self->ifaddr)) < 0) {
    perror("getifaddrs");
    return(1);
  }
  self->ifa = self->ifaddr;
  return(0);
#endif
  return(1);
}

void z_ifaddr_scanner_close (z_ifaddr_scanner_t *self) {
#if defined(Z_SYS_HAS_IFADDRS)
  freeifaddrs(self->ifaddr);
#endif
}

int z_ifaddr_scanner_get (z_ifaddr_scanner_t *self,
                          int *family,
                          char host[NI_MAXHOST])
{
#if defined(Z_SYS_HAS_IFADDRS)
  int r;
  *family = self->ifa->ifa_addr->sa_family;
  r = getnameinfo(self->ifa->ifa_addr,
                    (*family == AF_INET) ?
                        sizeof(struct sockaddr_in) :
                        sizeof(struct sockaddr_in6),
                      host, NI_MAXHOST,
                      NULL, 0, NI_NUMERICHOST);
  return(r);
#endif
  return(-1);
}

int z_ifaddr_scanner_next (z_ifaddr_scanner_t *self) {
#if defined(Z_SYS_HAS_IFADDRS)
  while (self->ifa != NULL) {
    self->ifa = self->ifa->ifa_next;
    if (self->ifa == NULL || self->ifa->ifa_addr == NULL)
      continue;
    return(1);
  }
#endif
  return(0);
}
