
#include <stdio.h>
#include <mqueue.h>
#include <string.h>

int main() {

    struct mq_attr attr;

    attr.mq_maxmsg = 5;
    attr.mq_msgsize = 512;

    mqd_t p = mq_open("/fun", O_CREAT | O_RDWR, 0664, &attr);
    if (p == -1) {
        perror("failed init");
        return 1;
    }

    char buf[512];
    memset(buf, 0, sizeof(buf));
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
