all: OBJS_DEST/depend

CC=g++

SRCS=${shell find src -type f -name "*.cpp"}

DEPENDS = $(patsubst %.cpp, OBJS_DEST/%.depend, $(SRCS))

OBJS_DEST/%.depend: %.cpp
	@echo -n OBJS_DEST/ > $@
	@dirname $< | tr -d "\n" >> $@
	@echo -n / >> $@
	@$(CC) -E -MM $< -I./ >> $@
	@echo depend $<

OBJS_DEST/depend: $(DEPENDS)
	@cat $(DEPENDS) > OBJS_DEST/depend

