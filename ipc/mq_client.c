/*
 * This program sends a message on the /fun message queue that then can be recieved by
 * another process "listening" in on the same message queue.
 *
 * The mq_send call does not block unless the queue is full.
 */
#include <stdio.h>
#include <mqueue.h>
#include <string.h>

int main() {

    struct mq_attr attr = {
        .mq_maxmsg = 5;
        .mq_msgsize = 512;
    };

    // the identifier name must start with a /
    mqd_t p = mq_open("/fun", O_CREAT | O_RDWR, 0664, &attr);
    if (p == -1) {
        perror("failed init");
        return 1;
    }

    char buf[512] = {0};
    strcpy((&buf[0]), "Hello world!");

    unsigned int prio = 1;

    ssize_t s = mq_send(p, (&buf[0]), sizeof(buf), prio);
    if (s == -1) {
        perror("failed to receive");
        return 1;
    }

    printf("sendt\n");

    mq_close(p);
    return 0;
}
