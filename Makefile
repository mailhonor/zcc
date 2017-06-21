
all: libzcc

.PHONY: test sample

DIRS=${shell find src -type d}
DIRS_DEST = $(patsubst %, OBJS_DEST/%, $(DIRS))
${shell mkdir -p $(DIRS_DEST)}

libzcc lib zcc: depend
	make -f makefiles/lib.makefile

test sample: libzcc
	make -f makefiles/sample_list.makefile

depend:
	make -f makefiles/depend.makefile

tag tags:
	ctags -R src/ zcc.h

clean:
	make -f makefiles/clean.makefile
	make clean -f makefiles/sample_list.makefile
	rm -f libzcc.a

CLEAN: clean
	rm -r tags plist

indent:
	astyle --style=kr `find src -name "*.cpp"`  > /dev/null
	astyle --style=kr `find sample -name "*.cpp"` > /dev/null 
	#astyle --style=kr `find src -name "*.h"` > /dev/null
	#astyle --style=kr `find sample -name "*.h"` > /dev/null
	astyle --style=kr  zcc.h > /dev/null
