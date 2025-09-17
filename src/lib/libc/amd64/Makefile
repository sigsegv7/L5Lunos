.SILENT:
override PROMPT := printf "%s\t\t%s\n"

ASMFILES = $(shell find . -name "*.S")
ASMOBJECTS = $(ASMFILES:.S=.S.o)

.PHONY: all
all: ../build/$(OBJECTS) ../build/$(ASMOBJECTS)

-include $(DEPS)
../build/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@

-include $(ASMDEPS)
../build/%.S.o: %.S
	mkdir -p $(@D)
	$(PROMPT) " MD.AS " $<
	$(CC) -c $< -o $@ $(CFLAGS) $(MD_CFLAGS)
