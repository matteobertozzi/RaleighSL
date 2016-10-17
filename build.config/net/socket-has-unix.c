#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <netdb.h>

int main (int argc, char **argv) {
    struct sockaddr_un addr;
    int sock;

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return(1);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "test.sock");

    close(sock);
    return(0);
}
