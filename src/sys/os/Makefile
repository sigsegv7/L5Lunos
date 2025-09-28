include ../conf/sys.mk

.SILENT:
override PROMPT := printf "%s\t\t%s\n"

CONF := $(shell cat ../conf/GENERIC | $(TOOLS)/kconf/kconf)
CFLAGS = -I../include/ -I../include/lib/ -I../target/header/ $(MI_CFLAGS) -O0 $(CONF) \
		 -target x86_64-elf
CFILES = $(shell find . -name "*.c")
CFILES += $(shell find ../vm -name "*.c")
CFILES += $(shell find ../lib -name "*.c")
CFILES += $(shell find ../io -name "*.c")
CFILES += $(shell find ../acpi -name "*.c")
CFILES += $(shell find ../compat -name "*.c")
CFILES += $(shell find ../fs -name "*.c")

DEPS = $(CFILES:.c=.d)
OBJECTS = $(CFILES:%.c=%.o)

.PHONY: all
all: $(OBJECTS)
	$(PROMPT) " MI.AR " $<

-include $(DEPS)
%.o: %.c
	$(PROMPT) " MI.CC " $<
	$(CC) -c $(CFLAGS) $(CONF) $< -o $@

.PHONY: clean
clean:
	rm -f $(DEPS) $(OBJECTS)
