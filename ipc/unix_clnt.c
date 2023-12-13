
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static const char g_sockname[] = "/tmp/1234.sock";

int unixSocketConnect(const char *path) {
    int sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock == -1) {
        return -1;
    }

    struct sockaddr_un addr = { 0 };

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1); 
    
    int rc = connect(sock, (struct sockaddr const*) &addr, sizeof(addr));
    if (rc == -1) {
        goto error;
    }

    return sock;
error:
    close(sock);
    return -1;
}

int main() {

    int sock = unixSocketConnect(g_sockname);
    if (sock == -1) {
        perror("connect");
        return 1;
    }

    char msg[] = "Hello there!";
    printf("sending '%s'\n", msg);
    int rc = write(sock, &msg, sizeof(msg));
    if (rc == -1) {
        perror("write");
        return 1;
    }

    close(sock);
    return 0;
}

