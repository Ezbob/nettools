/**
 * chat.c -- a little epoll chat server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034"

#define handle_perror(err_msg) do { perror(err_msg); exit(EXIT_FAILURE); } while(0)
#define handle_error(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); } while(0)

void *get_in_addr(struct sockaddr *sa) {
    if ( sa->sa_family == AF_INET ) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

/**
 *  Create a TCP listener socket, that listens on a valid localhost
 *  IP address.
 */
int get_listener_socket(char *port, int backlog) {
    int listener;
    int yes = 1;
    int rv;

    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC; // use either IPv6 or IPv4 addresses
    hints.ai_socktype = SOCK_STREAM; // streaming socket is a TCP socket (most likely)
    hints.ai_flags = AI_PASSIVE; // we are listening and accepting any connections (acting as a server)

    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) { // NULL is required for server mode
        handle_error("selectserver: %s\n", gai_strerror(rv));
    }

    /*
     * Select a address to create and bind a socket to.
     * getaddrinfo returns a linked list of possible address that can be bound to
     */
    for ( p = ai; p != NULL; p = p->ai_next ) {

        // creating the socket using the socket family, socktype and protocol
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            // error: Could not bind to address. we can use perror here to get a why-string
            continue;
        }

        // we can bind to local addresses except when it is already in use
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        // bind our socket to an address
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    if (p == NULL) {
        // end of linked list reached :(
        return -1;
    }

    // deallocate the address info that we got for getaddrinfo
    freeaddrinfo(ai);

    // Backlog is 10 and start the actual listening.
    // Backlog is the number of maximum of on-hold connections when listening
    if (listen(listener, backlog) == -1) {
        perror("listen");
        return -1;
    }

    return listener;
}


void add_to_events(struct epoll_event *events[], int epollfd, int newfd, int *fd_count, int *fd_size) {

    if ( *fd_count == *fd_size ) {
        // double the size of the array if we are out of space
        *fd_size *= 2;
        *events = realloc(*events, sizeof(**events) * (*fd_size));
        if (*events == NULL) {
            handle_perror("events realloc");
        }
    }

    struct epoll_event *new_event = (*events) + *fd_count;
    new_event->data.fd = newfd;
    new_event->events = EPOLLIN;

    if ( epoll_ctl(epollfd, EPOLL_CTL_ADD, newfd, new_event) == -1 ) {
        handle_perror("epoll_ctl; add");
    }

    (*fd_count)++;
}


void del_from_events(struct epoll_event events[], int epollfd, int fd, int i, int *fd_count) {

    if ( epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, events + i) == -1 ) {
        handle_perror("epoll_ctl; del");
    }

    (*fd_count)--;

    int next_fd_size = (*fd_count);
    for (int j = i; j < next_fd_size; ++j) {
       events[j] = events[j + 1]; // moving (left <- right)
    }

}


int main(void) {

    int listener, newfd, epollfd, epoll_count;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;

    char buf[256], remoteIP[INET6_ADDRSTRLEN];

    listener = get_listener_socket(PORT, 10);
    if (listener == -1) {
        handle_error("Error getting a listening socket. Exiting\n");
    }

    int fd_count = 0, fd_size = 10;
    struct epoll_event *events = malloc(sizeof(*events) * fd_size);

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        handle_perror("epollfd create1");
    }

    printf("chat: Listening on '%s'\n", PORT);
    add_to_events(&events, epollfd, listener, &fd_count, &fd_size);

    for (;;) {
        epoll_count = epoll_wait(epollfd, events, fd_count, -1);
        if (epoll_count == -1) {
            handle_perror("epoll_wait");
        }

        for (int i = 0; i < fd_count; ++i) {
            if (!(events[i].events & EPOLLIN)) continue;

            if (events[i].data.fd == listener) {
                addrlen = sizeof remoteaddr;
                newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
                if (newfd == -1) {
                    perror("accept");
                } else {
                    add_to_events(&events, epollfd, newfd, &fd_count, &fd_size);

                    printf("chat: new connection from %s on socket %d\n",
                        inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr *) &remoteaddr), remoteIP, INET6_ADDRSTRLEN),
                        newfd
                    );
                }
            } else {
                int sender_fd = events[i].data.fd;
                int nbytes = recv(sender_fd, buf, sizeof(buf), 0);

                if (nbytes <= 0) {
                    // connection closed
                    if (nbytes == 0) { // hung up
                        printf("chat: socket %d hung up\n", sender_fd);
                    } else { // error happend
                        perror("recv");
                    }

                    del_from_events(events, epollfd, sender_fd, i, &fd_count);
                    close(sender_fd);
                } else {
                    // broadcast to call other file descriptors
                    for (int j = 0; j < fd_count; j++) {
                        int dest_fd = events[j].data.fd;

                        if (dest_fd != listener && dest_fd != sender_fd) {
                            if (send(dest_fd, buf, nbytes, 0) == -1) {
                                perror("send");
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

