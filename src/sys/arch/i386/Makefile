include ../../conf/sys.mk

.SILENT:
override PROMPT := printf "%s\t\t%s\n"

TARGET = ../../l5
TARGET_INC = -I../../target/header/
CFLAGS = -I../../include/ -I../../include/lib/ $(TARGET_INC) $(MI_CFLAGS) $(MD_CFLAGS)
CFILES = $(shell find . -name "*.c")

ASMFILES = $(shell find . -name "*.S")
ASMOBJECTS = $(ASMFILES:.S=.S.o)

DEPS = $(CFILES:.c=.d)
ASMDEPS = $(ASMFILES:.S=.S.d)
OBJECTS = $(CFILES:%.c=%.o)
LD_FLAGS = -Tconf/sys.ld -L../../target -lkern

.PHONY: all
all: $(OBJECTS) $(ASMOBJECTS)
	$(LD) $(OBJECTS) $(ASMOBJECTS) -o $(TARGET) $(LD_FLAGS)

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
