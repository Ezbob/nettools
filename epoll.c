#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#define MAX_EVENTS 1

int main() {
    struct epoll_event event, events[MAX_EVENTS];
    int epollfd, listen_sock = 0, nfds, conn_sock;

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    event.events = EPOLLIN;
    event.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    printf("Hit RETURN or wait 2.5 seconds for timeout\n");

    //for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, 2500);

        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        if (nfds == 0) {
            printf("No events found\n");
        } else {
            int epollin_happend = events[0].events & EPOLLIN;
            if (epollin_happend) {
                printf("input event happend for fd %i\n", events[0].data.fd);
            } else {
                printf("another event happend => %i\n", events[0].events);
            } 
        }
    //}
    
    return 0;
}

