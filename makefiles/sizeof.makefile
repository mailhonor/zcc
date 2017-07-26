all: target

CC=g++
CFLAGS= -I./ -D__ZCC_SIZEOF_PROBE__ -ggdb
LIBS= ./libzcc.a -lm -lrt -lssl -lcrypto -lpthread

CPPS=src/stdlib/event_base.cpp \
     src/http/url.cpp \
     src/http/httpd.cpp

target: ./libzcc.a ./zcc.h
	@set -e; \
	for i in $(CPPS); \
	do (set -e; $(CC) $$i -o /tmp/sizeof $(CFLAGS) $(LIBS);/tmp/sizeof)||exit 1;\
	done;



