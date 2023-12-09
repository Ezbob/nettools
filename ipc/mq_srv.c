/*
 * Blocking example of a program receiving messages on a message queue named "/fun".
 * Receiving can be non-blocking by add the O_NONBLOCK to the open call, and then check
 * for the EAGAIN error produced by the mq_receive call (which is returned if the call otherwise
 * would have blocked).
 */
#include <stdio.h>
#include <mqueue.h>

int main() {
    mqd_t p = mq_open("/fun", O_CREAT | O_RDONLY, 0664, &((struct mq_attr) {
        .mq_maxmsg = 5,
        .mq_msgsize = 512
    }));
    if (p == -1) {
        perror("failed init");
        return 1;
    }

    char buf[512] = {0};

    unsigned int prio = 1;

    for(;;) {
        ssize_t s = mq_receive(p, (&buf[0]), sizeof(buf), &prio);
        if (s == -1) {
            perror("failed to receive");
            return 1;
        }

        printf("got %li bytes\n", s);
        printf("--> %s\n", (&buf[0]));
    }

    mq_close(p);
    return 0;
}

