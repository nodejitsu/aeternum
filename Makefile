CFLAGS=-g -Wall -Ideps/libuv/include -Ideps/saneopt/include

uname_S=$(shell uname -s)

OBJS += src/aeternum.c

ifeq (Darwin, $(uname_S))
CFLAGS+=-framework CoreServices
endif

ifeq (Linux, $(uname_S))
LDFLAGS=-lc -lrt -ldl -lm -pthread
endif

ifeq (SunOS, $(uname_S))
CFLAGS+=-lsendfile -lsocket -lkstat -lnsl -lm
endif

all: libuv libsaneopt aeternum

src/%.o: src/%.c
	gcc $(CFLAGS) -c $< -o $@

aeternum: $(OBJS)
	gcc $^ -O2 $(CFLAGS) deps/saneopt/libsaneopt.a deps/libuv/libuv.a $(LDFLAGS) -o $@

libuv:
	$(MAKE) -C deps/libuv/

libsaneopt:
	$(MAKE) -C deps/saneopt/

clean:
	rm -f aeternum
	rm -f aeternum_g

cleanall:
	rm -f aeternum
	rm -f aeternum_g
	$(MAKE) clean -C deps/libuv/

.PHONY: clean cleanall
