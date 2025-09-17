include ../../data/build/user.mk

CFILES = $(shell find . -name "*.c")
CFILES = $(shell find . -name "*.c")
OBJECTS = $(CFILES:%.c=%.o)

$(SYSROOT)/usr/bin/test: $(OBJECTS)
	$(LD) $(OBJECTS) -o $@ $(INTERNAL_CFLAGS)

%.o: %.c
	$(CC) $(INTERNAL_CFLAGS) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.d
