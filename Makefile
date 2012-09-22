
CFLAGS=-g -Wall -Ideps/libuv/include deps/libuv/uv.a

uname_S=$(shell uname -s)

ifeq (Darwin, $(uname_S))
CFLAGS+=-framework CoreServices
endif

ifeq (Linux, $(uname_S))
CFLAGS+=-lrt -ldl -lm -lev -pthread
endif

ifeq (SunOS, $(uname_S))
# This still needs to be finished - does not compile yet.
CFLAGS+=-m64
endif

all: libuv aeternum

aeternum: 
	gcc $(CFLAGS) -o aeternum aeternum.c options.c

libuv: 
	make -C deps/libuv/

clean: 
	rm -f aeternum

cleanall: 
	rm -f aeternum
	make clean -C deps/libuv/

.PHONY: clean cleanall
