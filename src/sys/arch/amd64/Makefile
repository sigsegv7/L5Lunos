include ../../conf/sys.mk

override PROMPT := printf "%s\t\t%s\n"

CONF := $(shell cat conf/GENERIC | $(TOOLS)/kconf/kconf)
TARGET_BIN = ../../l5
TARGET_INC = -I../../target/header/
CFLAGS = -I../../include/ -I../../include/lib/ $(TARGET_INC) $(MI_CFLAGS) $(MD_CFLAGS) $(CONF) \
		 -target x86_64-elf
CFILES = $(shell find . -name "*.c")

ASMFILES = $(shell find . -name "*.S")
ASMOBJECTS = $(ASMFILES:.S=.S.o)

DEPS = $(CFILES:.c=.d)
ASMDEPS = $(ASMFILES:.S=.S.d)
OBJECTS = $(CFILES:%.c=%.o)
LD_FLAGS = -Tconf/sys.ld

.PHONY: all
all: $(OBJECTS) $(ASMOBJECTS)

-include $(DEPS)
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

-include $(ASMDEPS)
%.S.o: %.S
	$(PROMPT) " MD.AS " $<
	$(CC) -c $< -o $@ $(CFLAGS) $(MD_CFLAGS)

.PHONY: clean
clean:
	rm -f $(DEPS) $(ASMDEPS) $(OBJECTS) $(ASMOBJECTS)
