LDSCRIPT=../../sys/arch/$(TARGET)/conf/user.ld

.PHONY: all
all:
	cd libc/; LDSCRIPT=$(LDSCRIPT) CC=$(CC) ASM=$(ASM) \
		LD=$(ASM) SYSROOT=$(SYSROOT) make
	cd libwidget/; LDSCRIPT=$(LDSCRIPT) CC=$(CC) ASM=$(ASM) \
		LD=$(ASM) SYSROOT=$(SYSROOT) make

.PHONY: clean
clean:
	cd libc/; make clean
