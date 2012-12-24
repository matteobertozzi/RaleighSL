#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

int main (int argc, char **argv) {
    int res = accept4(0, NULL, NULL, SOCK_NONBLOCK);
    (void)res;
    return(0);
}
