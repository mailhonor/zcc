all: libzcc.a libzcc_coroutine.a

include OBJS_DEST/depend

CC=g++

CFLAGS= -Wall -I./ -O3 -g

SRCS_COROUTINE=${shell find src -type f -name "*.cpp"}
OBJS_COROUTINE = $(patsubst %.cpp, OBJS_DEST/%.o, $(SRCS_COROUTINE))


SRCS_ZCC=${shell find src -type f -name "*.cpp"|grep -v "^src/coroutine/"}
OBJS_ZCC = $(patsubst %.cpp, OBJS_DEST/%.o, $(SRCS_ZCC))

OBJS_DEST/%.o: %.cpp
	@echo build $<
	@$(CC) $(CFLAGS) -c $< -o $@


libzcc.a: $(OBJS_ZCC)
	@echo build libzcc.a
	@ar r libzcc.a $(OBJS_ZCC)
	@ranlib libzcc.a

libzcc_coroutine.a: $(OBJS_COROUTINE)
	@echo build libzcc_coroutine.a
	@ar r libzcc_coroutine.a $(OBJS_COROUTINE)
	@ranlib libzcc_coroutine.a

