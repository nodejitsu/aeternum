CFLAGS=-g -Wall -Ideps/libuv/include

uname_S=$(shell uname -s)

ifeq (Darwin, $(uname_S))
CFLAGS+=-framework CoreServices
endif

ifeq (Linux, $(uname_S))
CFLAGS+=-lrt -ldl -lm -lev -pthread
endif

ifeq (SunOS, $(uname_S))
CFLAGS+=-m32 -lsocket -lkstat -lnsl -lm
endif

all: libuv aeternum

aeternum: 
	gcc $(CFLAGS) -o aeternum aeternum.c options.c deps/libuv/uv.a

libuv: 
	make -C deps/libuv/

clean: 
	rm -f aeternum

cleanall: 
	rm -f aeternum
	make clean -C deps/libuv/

.PHONY: clean cleanall
