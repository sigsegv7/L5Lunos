TARGET = x86_64
SHIMDIR = shim
SHIMBIN = BOOTX64.EFI
SYSROOT = root
OMAR = tools/omar/bin/omar
ISO = lunos.iso

# QEMU emulator flags
QEMU_FLAGS = --enable-kvm -serial stdio -cdrom $(ISO) \
			-M q35 -cpu host -smp 4 -m 2G

.PHONY: all
all: root image

root:
	mkdir -p root/
	mkdir -p root/boot/

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
	qemu-system-$(TARGET) $(QEMU_FLAGS)

.PHONY: clean
clean:
	rm -f lunos.iso
