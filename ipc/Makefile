
CFLAGS+=-Wall -Wextra -std=c99 -pedantic
LDLIBS+=-lrt

TARGETS=mq_client mq_srv shm_reader shm_sender unix_srv unix_clnt

all: $(TARGETS)

clean:
	$(RM) $(TARGETS)
