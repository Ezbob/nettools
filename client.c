#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490"

#define MAXDATASIZE 100

#define DIE(...) do { fprintf(stderr, __VA_ARGS__); exit(1); } while(0)
#define PDIE(arg) do { perror(arg); exit(1); } while(0)

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *serverinfo, *p;

    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        DIE("usage: client hostname\n"); 
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &serverinfo)) != 0) {
        DIE("getaddrinfo: %s\n", gai_strerror(rv));
    }

    for (p = serverinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        DIE("client: failed to connect\n");
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    
    freeaddrinfo(serverinfo);

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        PDIE("recv");
    }

    buf[numbytes] = '\0';
    
    printf("client: received '%s'\n", buf);

    close(sockfd);

    return 0;
}

