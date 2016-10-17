#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return(1);
  }

  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;
    printf("%-8s %s (%d)\n",
           ifa->ifa_name,
           //(family == AF_PACKET) ? "AF_PACKET" :
           (family == AF_INET)   ? "AF_INET" :
           (family == AF_INET6)  ? "AF_INET6" : "???",
           family);

    if (family == AF_INET || family == AF_INET6) {
      s = getnameinfo(ifa->ifa_addr,
                      (family == AF_INET) ?
                        sizeof(struct sockaddr_in) :
                        sizeof(struct sockaddr_in6),
                      host, NI_MAXHOST,
                      NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        return(1);
      }

      printf("\t\taddress: <%s>\n", host);
    }
  }

  freeifaddrs(ifaddr);
  return(0);
}