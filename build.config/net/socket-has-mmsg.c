#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>

int main (int argc, char **argv) {
  struct sockaddr_in sa;
  struct mmsghdr msg[2];
  struct iovec msg1[2], msg2;
  int sockfd;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) {
    perror("socket()");
    return(1);
  }

  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  sa.sin_port = htons(1234);
  if (connect(sockfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
    perror("connect()");
    return(1);
  }

  memset(msg1, 0, sizeof(msg1));
  msg1[0].iov_base = "one";
  msg1[0].iov_len = 3;
  msg1[1].iov_base = "two";
  msg1[1].iov_len = 3;

  memset(&msg2, 0, sizeof(msg2));
  msg2.iov_base = "three";
  msg2.iov_len = 5;

  memset(msg, 0, sizeof(msg));
  msg[0].msg_hdr.msg_iov = msg1;
  msg[0].msg_hdr.msg_iovlen = 2;

  msg[1].msg_hdr.msg_iov = &msg2;
  msg[1].msg_hdr.msg_iovlen = 1;

  sendmmsg(sockfd, msg, 2, 0);

  return(0);
}
