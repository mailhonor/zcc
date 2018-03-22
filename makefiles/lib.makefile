all: libzcc.a libzcc_coroutine.a

include OBJS_DEST/depend

CC=g++

#-Wcast-qual -Wshadow -Wmissing-declarations
CFLAGS= -I./ -std=gnu++11 -ggdb -O3 -D___ZCC_INNER___ -D_GNU_SOURCE \
	-DLINUX2 -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT -D_USE_FAST_MACRO \
	-Wabi -Waddress -Wall -Wbuiltin-macro-redefined -Wcast-align -Wchar-subscripts -Wclobbered  \
	-Wcomment -Wcomments -Wctor-dtor-privacy -Wdeprecated -Wdiv-by-zero -Wendif-labels \
	-Wenum-compare -Wextra -Wfatal-errors -Wfloat-equal -Wformat -Wignored-qualifiers -Winit-self \
	-Winline -Winvalid-pch -Wmain -Wmissing-field-initializers -Wmissing-format-attribute \
	-Wmissing-include-dirs -Wmultichar -Wno-long-long -Wnon-template-friend -Wnon-virtual-dtor \
	-Woverlength-strings -Woverloaded-virtual -Wpacked-bitfield-compat -Wparentheses -Wpmf-conversions \
	-Wpointer-arith -Wpragmas -Wredundant-decls -Wreorder -Wreturn-type -Wsequence-point -Wsign-compare \
	-Wsign-promo -Wstrict-null-sentinel -Wsync-nand -Wsynth -Wtrigraphs -Wuninitialized -Wunknown-pragmas \
	-Wvariadic-macros -Wvla -Wvolatile-register-var -Wwrite-strings

CFLAGS= -I./ -std=gnu++11 -ggdb -O3 -D___ZCC_INNER___ -D_GNU_SOURCE 

SRCS_COROUTINE=${shell find src/coroutine/ -type f -name "*.cpp"}
OBJS_COROUTINE = $(patsubst %.cpp, OBJS_DEST/%.o, $(SRCS_COROUTINE))


SRCS_ZCC=${shell find src -type f -name "*.cpp"|grep -v "^src/coroutine/"}
OBJS_ZCC = $(patsubst %.cpp, OBJS_DEST/%.o, $(SRCS_ZCC))

OBJS_DEST/%.o: %.cpp
	@echo build $<
	@$(CC) -c $< -o $@ $(CFLAGS)


libzcc.a: $(OBJS_ZCC)
	@echo build libzcc.a
	@ar r libzcc.a $(OBJS_ZCC)
	@ranlib libzcc.a

libzcc_coroutine.a: $(OBJS_COROUTINE)
	@echo build libzcc_coroutine.a
	@ar r libzcc_coroutine.a $(OBJS_COROUTINE)
	@ranlib libzcc_coroutine.a

