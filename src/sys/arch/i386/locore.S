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

fbtag_start:
    .word 5                         // Type
    .word 1                         // Flags
    .long fbtag_end - fbtag_start   // Length
    .long 800                       // Width
    .long 600                       // Height
    .long 32                        // Depth
fbtag_end:

.align 8
endtag:
    .word 0
    .word 0
    .word 8

    .text
    .code32
    .globl _start
_start:
    jmp .
