#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_EVENTS  2

int set_nonblock(int fd) {
    int flags = 0;

    flags = fcntl(fd, F_GETFL);
    flags = flags | O_NONBLOCK;

    return fcntl(fd, F_SETFL, flags);
}

int main() {
    struct epoll_event event, events[MAX_EVENTS];
    int epollfd, listen_sock = 0, nfds;

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    if (set_nonblock(listen_sock)) {
        perror("failed to set nonblock");
        exit(EXIT_FAILURE);
    }

    event.events = EPOLLIN | EPOLLET; // EPOLLET: edge trigger, that is any new event is only broadcasted once
                                      // you will need to read or write all data untill either EAGAIN or EWOULDBLOCK 
                                      // (given that the fd is set to non-block) 
    event.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL); // this blocks signal SIGINT default handler

    int sfd = signalfd(-1, &mask, SFD_NONBLOCK); // using signal fd to catch process signals in the epoll loop
    if (sfd == -1) {
        perror("signal: failed to create signalfd");
        exit(EXIT_FAILURE);
    }

    event.events = EPOLLIN | EPOLLET;
    event.data.fd = sfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &event) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
    

    printf("type something...\n");

    uint8_t readbuf[1024] = { 0 };

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            int can_read = events[i].events & (EPOLLIN | EPOLLERR); // EPOLLERR needs to be handled
            int can_write = events[i].events & (EPOLLOUT | EPOLLERR); // you can write and read on any notified fd

            if (can_read) {
                printf("input event happend for fd %i\n", events[i].data.fd);
                if (events[i].data.fd == sfd) {
                    printf("signalled!\n");
                    exit(0);
                } else {
                    int nread = 0; 
                    printf("I saw '");

                    nread = read(events[i].data.fd, readbuf, sizeof(readbuf) - 1);
                    while (nread > 0) {
                        printf("%.*s", nread - 1, readbuf);
                        nread = read(events[i].data.fd, readbuf, sizeof(readbuf) - 1);
                        if (nread < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                            break;
                        }
                    };
                    printf("'\n"); 
                }
            }
            if (can_write) {
                printf("writable event happend => %i\n", events[i].events);
            } 
        }
    }

    close(epollfd);
    
    return 0;
}

