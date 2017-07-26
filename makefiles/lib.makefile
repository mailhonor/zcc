all: libzcc.a

include OBJS_DEST/depend

CC=g++

CFLAGS= -Wall -I./ -O3 -g

SRCS=${shell find src -type f -name "*.cpp"}

OBJS_DEST = $(patsubst %.cpp, OBJS_DEST/%.o, $(SRCS))

OBJS_DEST/%.o: %.cpp
	@echo build $<
	@$(CC) $(CFLAGS) -c $< -o $@

libzcc.a: $(OBJS_DEST)
	@echo build libzcc.a
	@ar r libzcc.a $(OBJS_DEST)
	@ranlib libzcc.a

