#include <zcl/socket.h>

int main (int argc, char **argv) {
    char buffer[32];
    ssize_t n;
    int sock;
    int i;

    sock = z_socket_unix_connect("benedikte");
    z_socket_tcp_set_nodelay(sock);
    for (i = 0; i < 100000; ++i) {
        n = write(sock, "Test 0\n", 7);
        n += read(sock, buffer, 32);
    }

    close(sock);
    return(0);
}
