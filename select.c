#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

int main(void) {

    int retval;
    fd_set rfds;
    struct timeval tv;

    for (;;) {
       FD_ZERO(&rfds);
       FD_SET(0, &rfds);

       tv.tv_sec = 10;
       tv.tv_usec = 0;

       retval = select(1, &rfds, NULL, NULL, &tv);
       if (retval == -1) {
           perror("select()");
       } else if (retval) {
           if (FD_ISSET(0, &rfds)) {
               printf("got something \n");
               char b[1024];
               b[1023] = '\0';
               ssize_t nread = read(0, &b, 1024);
               if (nread < 0) {
                    perror("read");
                    return 1;
                }
               printf("read this '%.*s'\n", (int)nread, b);
           }
       } else {
           printf("No data\n");
       }
    }

    return 0;
}
