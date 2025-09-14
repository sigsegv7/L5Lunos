TARGET = amd64
KBIN = sys/l5
SHIMDIR = shim
SHIMBIN = BOOTX64.EFI
SYSROOT = root
OMAR = tools/omar/bin/omar
ISO = lunos.iso

# Build tools
BINUTILS_DIR := $(shell pwd)/cc/toolchain/build-binutils
CC := $(shell pwd)/cc/gcc/bin/x86_64-pc-ethos-gcc
LD := $(BINUTILS_DIR)/bin/x86_64-pc-ethos-ld
AS := $(BINUTILS_DIR)/bin/x86_64-pc-ethos-as

# QEMU emulator flags
QEMU_FLAGS = --enable-kvm -serial stdio -cdrom $(ISO) \
			-M q35 -cpu host -smp 4 -m 2G

.PHONY: all
all: root sys image

.PHONY: sys
sys:
	cd sys/; make CC=$(CC) AS=$(AS) LD=$(LD) TARGET=$(TARGET)
	cp $(KBIN) root/boot/

root:
	mkdir -p $(SYSROOT)/
	mkdir -p $(SYSROOT)/boot/
	mkdir -p $(SYSROOT)/usr/include/
	mkdir -p sys/target/header/
	rsync -av sys/include/arch/$(TARGET)/ sys/target/header/machine/

.PHONY: sysroot
image:
	$(OMAR) -i $(SYSROOT) -o $(SYSROOT)/boot/initrd.omar
	cp $(SHIMDIR)/limine/limine-bios-cd.bin $(SYSROOT)/
	cp $(SHIMDIR)/limine/limine-uefi-cd.bin $(SYSROOT)/
	cp $(SHIMDIR)/limine/limine-bios.sys $(SYSROOT)/
	cp data/boot/limine.conf $(SYSROOT)/boot/limine.conf
	xorriso -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4\
		-boot-info-table --efi-boot limine-uefi-cd.bin -efi-boot-part \
		--efi-boot-image --protective-msdos-label $(SYSROOT) -o $(ISO) 1>/dev/null
	$(SHIMDIR)/limine/limine bios-install $(ISO) 1>/dev/null


.PHONY: qemu-amd64
run:
	qemu-system-x86_64 $(QEMU_FLAGS)

.PHONY: clean
clean:
	rm -rf lunos.iso root
	rm -f $(KBIN)
	cd sys/; make clean TARGET=$(TARGET)

.PHONY: distclean
distclean:
	make clean; rm -rf srv/root
