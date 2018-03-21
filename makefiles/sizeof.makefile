all: target

CC=g++
CFLAGS= -I./ -D__ZCC_SIZEOF_PROBE__ -ggdb
LIBS= ./libzcc.a -lm -lrt -lssl -lcrypto -lpthread

CPPS= 

target: ./libzcc.a ./zcc.h
	@set -e; \
	for i in $(CPPS); \
	do (set -e; $(CC) $$i -o /tmp/sizeof $(CFLAGS) $(LIBS);/tmp/sizeof)||exit 1;\
	done;



