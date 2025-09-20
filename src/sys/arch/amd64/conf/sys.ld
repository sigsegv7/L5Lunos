OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(__bsp_entry)

PHDRS
{
    text          PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ;
    rodata        PT_LOAD    FLAGS((1 << 2)) ;
    data          PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ;
}

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text : {
        *(.text .text.*)
    } :text

    . += CONSTANT(MAXPAGESIZE);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .modules : {
        __modules_init_start = .;
        KEEP(*(.modules .modules))
        __modules_init_end = .;
    } :rodata

    . += CONSTANT(MAXPAGESIZE);

    .data : {
        *(.data)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    . = ALIGN(64);
    .data.cacheline_aligned : {
        *(.data.cacheline_aligned)
    }

    /DISCARD/ : {
        *(.eh_frame .eh_frame.*)
        *(.note .note.*)
    }
}
