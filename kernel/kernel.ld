OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(kentry)

SECTIONS
{
    . = 0x10000;
    .text       : {
        *(.entry)
        *(.text) 
    }
    .data       : { *(.data) }
    .bss        : { *(.bss) }
    .rodata     : { *(.rodata*) }
    _end = .;

    . = ASSERT(_end <= 0x9C000, "Kernel is too big!");
}