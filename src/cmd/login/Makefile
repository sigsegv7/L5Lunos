include ../../data/build/user.mk

CFILES = $(shell find . -name "*.c")
CFILES = $(shell find . -name "*.c")
CFLAGS = -L$(LIBC_DIR) -lc $(INTERNAL_CFLAGS) -L../../lib/libc/ -lc
OBJECTS = $(CFILES:%.c=%.o)

$(SYSROOT)/usr/sbin/login: $(OBJECTS)
	$(LD) $(OBJECTS) -o $@ $(CFLAGS)

%.o: %.c
	$(CC) $(INTERNAL_CFLAGS) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.d
