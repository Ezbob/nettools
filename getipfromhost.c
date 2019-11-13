#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {

    struct addrinfo hints, *results, *pointer;

    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: %s hostname\n", argv[0]);
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], NULL, &hints, &results)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    printf("IP addresses for %s:\n\n", argv[1]);

    for (pointer = results; pointer != NULL; pointer = pointer->ai_next) {
        void *addr;
        char *ipver;

        if (pointer->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) pointer->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) pointer->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        inet_ntop(pointer->ai_family, addr, ipstr, sizeof ipstr);
        printf("   %s: %s\n", ipver, ipstr); 
    }

    freeaddrinfo(results);
    return 0;
}


