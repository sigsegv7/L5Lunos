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
	cd reboot/; make LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR)
	cd fetch/; make LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR)
	cd lsdisk/; make LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR)
	cd login/; make LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR)
	cd echo/; make LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR)
	cd cat/; make LDSCRIPT=$(LDSCRIPT) CC=$(CC) AS=$(AS) LD=$(LD) SYSROOT=$(SYSROOT) \
		LIBC_DIR=$(shell pwd)/../$(LIBC_DIR)

.PHONY: clean
clean:
	cd init/; make clean
	cd hush/; make clean
	cd reboot/; make clean
	cd fetch/; make clean
	cd lsdisk/; make clean
	cd login/; make clean
	cd echo/; make clean
	cd cat/; make clean
