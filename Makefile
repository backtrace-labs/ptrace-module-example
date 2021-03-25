CC=gcc
CXX=g++
LD=gcc
CFLAGS=-D_DEFAULT_SOURCE -D_GNU_SOURCE -D_BSD_SOURCE -D__x86_64__ -std=gnu99 -Wall -W -Wundef -Wendif-labels -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Winline -Wdisabled-optimization -fstrict-aliasing -O2 -pipe -Wno-parentheses -ggdb -rdynamic -fno-omit-frame-pointer -m64 -fPIC
LDFLAGS=-shared -fPIC 

INCLUDE=-I../include -I../src -I/opt/backtrace/include -I/opt/backtrace/include/json-c -I/opt/backtrace/include/ck

example: pmodule-example.c
	$(CC) $(INCLUDE) $(CFLAGS) pmodule-example.c -c
	$(LD) $(LDFLAGS) pmodule-example.o -o pmodule-example.so

install: example
	cp pmodule-example.so /opt/backtrace/ptrace/modules/lib/

clean:
	rm pmodule-example.so
	rm pmodule-example.o

uninstall:
	rm /opt/backtrace/ptrace/modules/lib/pmodule-example.so
