CFLAGS+=-g -Wall -O2
LDLIBS=-levent_core -Wl,--rpath -Wl,/usr/local/lib

all: client server bench server-line-echo server-half-close

clean:
	rm -f client server bench server-line-echo server-half-close

.PHONY: all clean

