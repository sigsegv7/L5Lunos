LDSCRIPT=../../sys/arch/$(TARGET)/conf/user.ld
LIBC_DIR = lib/libc
CC =
AS =
LD =

.PHONY: all
all:
	cd test/; LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR) make

.PHONY: clean
clean:
	cd test/; make clean
