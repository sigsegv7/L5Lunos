LDSCRIPT=../../sys/arch/$(TARGET)/conf/user.ld

.PHONY: all
all:
	cd test/; LDSCRIPT=$(LDSCRIPT) CC=$(CC) ASM=$(ASM) \
		      LD=$(ASM) SYSROOT=$(SYSROOT) make

.PHONY: clean
clean:
	cd test/; make clean
