/**
 * chat.c -- a little epoll chat server
 */
#include <errno.h>
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
#include <fcntl.h>

#define PORT "9034"

#define handle_perror(err_msg) do { perror(err_msg); exit(EXIT_FAILURE); } while(0)
#define handle_error(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); } while(0)

#define is_non_block_error(errcode) (errcode == EAGAIN || errcode == EWOULDBLOCK)

void print_events(struct epoll_event events[], int fd_count) {
    for (int i = 0; i < fd_count; ++i) {
        printf("%i fd: %i, event: %i\n", i , events[i].data.fd, events[i].events);
    }
}


struct chat_room {
    int epollfd;
    struct epoll_event *events;
    int *broadcast_fds;
    int in_use;
    int capasity;
};

void debug_chat_room(struct chat_room *room) {
    for (int i = 0; i < room->in_use; ++i) {
        printf("%i| e.data.fd: %i, bfd: %i\n", i, room->events[i].data.fd, room->broadcast_fds[i]);
    }
}

int chat_room_init(struct chat_room *r, int initial_cap) {
    if (!r || initial_cap <= 0) return 1;

    r->events = malloc(sizeof(struct epoll_event) * initial_cap);
    if (!r->events) return 1;
    
    r->broadcast_fds = malloc(sizeof(int) * initial_cap);
    if (!r->broadcast_fds) return 1;

    r->epollfd = epoll_create1(0);
    if (r->epollfd == -1) return 1;

    r->in_use = 0;
    r->capasity = initial_cap;

    return 0;
};

void chat_room_deinit(struct chat_room *r) {
    if (!r) return;

    free(r->events);
    free(r->broadcast_fds);
}

int chat_room_grow(struct chat_room *r) {
    r->capasity *= 2;

    r->broadcast_fds = realloc(r->broadcast_fds, sizeof(int) * r->capasity);
    if (!r->broadcast_fds) return 1;

    r->events = realloc(r->events, sizeof(struct epoll_event) * r->capasity);
    if (!r->events) return 1;

    return 0;
}

void chat_room_add_client(struct chat_room *r, int newfd) {
    struct epoll_event *new_event = r->events;

    new_event->data.fd = newfd;
    new_event->events = EPOLLIN;

    if ( epoll_ctl(r->epollfd, EPOLL_CTL_ADD, newfd, new_event) == -1 ) {
        handle_perror("epoll_ctl; add");
    }

    r->broadcast_fds[r->in_use] = newfd;
    r->in_use++; 

    if (r->in_use == r->capasity) {
        chat_room_grow(r);
    }
}

void chat_room_remove_client(struct chat_room *r, int fd) {
    int i, j;

    if ( epoll_ctl(r->epollfd, EPOLL_CTL_DEL, fd, NULL) == -1 ) {
        handle_perror("epoll_ctl; del");
    }

    for (i = 0; i < r->in_use; ++i) {
        if (r->broadcast_fds[i] == fd) break;
    }

    for (j = i; j < (r->in_use - 1); ++j) {
        r->broadcast_fds[j] = r->broadcast_fds[j + 1];
    }

    r->in_use--;

} 

int chat_room_poll(struct chat_room *r, int timeout) {
    return epoll_wait(r->epollfd, r->events, r->capasity, timeout);
}

void chat_room_broadcast(struct chat_room *r, char *msg_buffer, int msg_size, int sender, int listener) {

    for (int j = 0; j < r->in_use; j++) {
        int destination = r->broadcast_fds[j];

        if (destination != listener && destination != sender) {
            if (send(destination, msg_buffer, msg_size, 0) == -1 && !is_non_block_error(errno)) {
                perror("send");
            }
        }
    }
}


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

int main(void) {

    int listener, newfd, epollfd, epoll_count, i;
    struct sockaddr_storage remoteaddr;
    struct chat_room chat_room;
    struct epoll_event *current_event;
    socklen_t addrlen;

    char buf[256], remoteIP[INET6_ADDRSTRLEN];

    listener = get_listener_socket(PORT, 10);
    if (listener == -1) {
        handle_error("Error getting a listening socket. Exiting\n");
    }

    chat_room_init(&chat_room, 10);

    printf("chat: Listening on '%s'\n", PORT);

    chat_room_add_client(&chat_room, listener);

    for (;;) {
        epoll_count = chat_room_poll(&chat_room, -1);
        if (epoll_count == -1) {
            handle_perror("epoll_wait");
        }

        for (i = 0; i < epoll_count; ++i) {
            struct epoll_event *current_event = chat_room.events + i;
            
            if (!(current_event->events & EPOLLIN)) continue;

            if (current_event->data.fd == listener) {
                addrlen = sizeof remoteaddr;
                newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
                if (newfd == -1) {
                    perror("accept");
                } else {
                    fcntl(newfd, F_SETFL, O_NONBLOCK);

                    //add_to_events(&events, epollfd, newfd, &fd_count, &fd_size);
                    chat_room_add_client(&chat_room, newfd);

                    printf("chat: new connection from %s on socket %d\n",
                        inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr *) &remoteaddr), remoteIP, INET6_ADDRSTRLEN),
                        newfd
                    );
                }
            } else {
                int sender_fd = current_event->data.fd;
                int nbytes = recv(sender_fd, buf, sizeof(buf), 0);

                if (nbytes <= 0) {
                    // connection closure state
                    if (nbytes == 0) {
                        // socket hung up
                        printf("chat: socket %d hung up\n", sender_fd);
                        chat_room_remove_client(&chat_room, sender_fd);
                        close(sender_fd);

                    } else if (!is_non_block_error(errno)) {
                        // error happend that is not the non-block error
                        perror("recv");
                        chat_room_remove_client(&chat_room, sender_fd);
                        close(sender_fd);
                    }
                } else {
                    // broadcast to call other file descriptors
                    chat_room_broadcast(&chat_room, buf, nbytes, sender_fd, listener);
                }
            }
        }
    }

    chat_room_deinit(&chat_room);
    return 0;
}

