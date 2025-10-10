LDSCRIPT=../../sys/arch/$(TARGET)/conf/user.ld
LIBC_DIR = lib/libc
CC =
AS =
LD =

.PHONY: all
all:
	cd init/; make LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR)
	cd hush/; make LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR)

.PHONY: clean
clean:
	cd init/; make clean
	cd hush/; make clean
