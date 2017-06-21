all: target

.PHONY: tags

CC=g++
CFLAGS= -ggdb -Wall -I../../ -O3
GLOBAL_LIBS=
SRCS=${shell find -type f -name "*.cpp"}
DEST := $(SRCS:.cpp=)

$(DEST): ../../libzcc.a
.cpp:
	$(CC) $*.cpp -o $* $(CFLAGS) ../../libzcc.a $(GLOBAL_LIBS) $($*_LIB) $(LIBS)

target: libzcc $(DEST)

clean: CLEAN
	@echo clean

CLEAN:
	@rm -f *~; rm -f $(DEST); rm -f tags gmon.out depend;rm -rf $(DELS);

libzcc:
	@echo build global lib
	@cd ../../; make libzcc

tag tags:
	cd ../../; make tags

targetFromTop: $(DEST)

