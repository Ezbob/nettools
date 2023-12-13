#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static const char g_sockname[] = "/tmp/1234.sock";

int listenUnixSocket(const char *path, int backlog) {
    int sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock == -1) {
        return -1;
    }

    struct sockaddr_un addr = { 0 };

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1); 
    
    int rc = bind(sock, (struct sockaddr const*) &addr, sizeof(addr));
    if (rc == -1) {
        goto error;
    }

    rc = listen(sock, backlog);
    if (rc == -1) {
        goto error;
    }

    return sock;

error:
    close(sock);
    unlink(path);
    return -1;
}



int main() {
    int sock = listenUnixSocket(g_sockname, 20);
    if (sock == -1) {
        perror("listen");
        return 1;
    }

    //for(;;) {
        int data_socket = accept(sock, NULL, NULL);
        if (data_socket == -1) {
            perror("accept");
            close(sock);
            unlink(g_sockname);
            return 1;
        }
        printf("got accepted\n");

        char buffer[1024] = {0};
        int rc = read(data_socket, buffer, sizeof(buffer) - 1);
        if (rc == -1) {
            perror("read");
            close(data_socket);
            close(sock);
            unlink(g_sockname);
            return 2;
        }

        printf("got %s\n", buffer);

        close(data_socket);
    //}
    
    close(sock);
    unlink(g_sockname);
    return 0;
}
