    .set MAGIC, 0xe85250d6
    .set ARCH,  0x00            // i386, protected mode
    .set LEN, hdr_end - hdr_start
    .set CHECKSUM, 0x100000000 - (0xe85250d6 + 0 + (hdr_end - hdr_start))

    .section .multiboot
    .align 4
hdr_start:
    .long MAGIC
    .long ARCH
    .long LEN
    .long CHECKSUM
hdr_end:

    .text
    .code32
    .globl _start
_start:
    jmp .
