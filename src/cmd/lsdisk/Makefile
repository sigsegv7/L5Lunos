include ../../data/build/user.mk

CFILES = $(shell find . -name "*.c")
CFILES = $(shell find . -name "*.c")
CFLAGS = -L$(LIBC_DIR) -lc $(INTERNAL_CFLAGS) \
		 -L../../lib/libwidget -lwidget -L../../lib/libc/ -lc \
		 -I../../lib/libwidget/include/
OBJECTS = $(CFILES:%.c=%.o)

$(SYSROOT)/usr/bin/lsdisk: $(OBJECTS)
	$(LD) $(OBJECTS) -o $@ $(CFLAGS)

%.o: %.c
	$(CC) $(INTERNAL_CFLAGS) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.d
