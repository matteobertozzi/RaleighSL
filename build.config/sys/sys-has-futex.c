#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <stdio.h>

int main (int argc, char **argv) {
  syscall(__NR_futex, NULL, FUTEX_WAIT_PRIVATE, 257, NULL, NULL, 0);
  return(1);
}
