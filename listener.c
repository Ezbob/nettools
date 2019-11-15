#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>


#define MYPORT "3490"
#define BACKLOG 210

#define die(...) do { fprintf(stderr,  __VA_ARGS__); exit(1); } while (0)
#define pdie(str) do { perror(str); exit(1); } while (0)


int main(void) {
    
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;
    char incoming[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int addr_info_code;
    if ( (addr_info_code = getaddrinfo(NULL, MYPORT, &hints, &res)) != 0 ) {
        die("Error: Could not get address info: %s\n", gai_strerror(addr_info_code));
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (sockfd == -1) {
        freeaddrinfo(res);
        pdie("Error: Could not create socket");       
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        freeaddrinfo(res);
        pdie("Error: Could not bind socket");
    }    

    freeaddrinfo(res);

    if (listen(sockfd, BACKLOG) == -1) {
        pdie("Error: Could not listen on socket");
    }
    
    addr_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    
    if (new_fd == -1) {
        freeaddrinfo(res);
        pdie("Error: Could not accept incoming socket");
    }

    close(sockfd);
    
    printf("New accept\n");

    // send stuff
    char *msg = "Anders was here!\n";
    int bytes_sent = send(new_fd, msg, strlen(msg), 0);

    // receive stuff

    char buffer[512];
    memset(&buffer, '\0', sizeof buffer);

    int bytes_received = recv(new_fd, &buffer, strlen(buffer), 0);

    if (bytes_received > 0) {
        printf("Recevied: %s\n", buffer);
    }

    close(new_fd);
    return 0;
}

