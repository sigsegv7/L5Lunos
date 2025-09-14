include ../conf/sys.mk

.SILENT:
override PROMPT := printf "%s\t\t%s\n"

TARGET_LIB = ../target/libkern.a
CFLAGS = -I../include/ -I../include/lib/ -I../target/header/ $(MI_CFLAGS) -O0
CFILES = $(shell find . -name "*.c")
CFILES += $(shell find ../lib -name "*.c")

DEPS = $(CFILES:.c=.d)
OBJECTS = $(CFILES:%.c=%.o)

.PHONY: all
all: $(OBJECTS)
	$(PROMPT) " MI.AR " $<
	ar rcs $(TARGET_LIB) $(OBJECTS)

-include $(DEPS)
%.o: %.c
	$(PROMPT) " MI.CC " $<
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(DEPS) $(OBJECTS)
