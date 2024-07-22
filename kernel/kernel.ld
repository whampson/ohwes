OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

SECTIONS
{
    __page_shift = 12;
    __page_size = (1 << __page_shift);
    __page_offset = 0xC0000000;

    __kernel_base = __page_offset;

    __setup_lma = 0x10000;
    .setup __setup_lma :
    {
        *(.setup)
    }
    . = ALIGN(__page_size);

    __text_lma = .;
    .text __page_offset + __text_lma : AT (__text_lma)
    {
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        *(.bss*)
    }

    .eh_frame :
    {
        *(.eh_frame*)
    }

    . = ALIGN(__page_size);
    __kernel_pages = (. - __kernel_base) >> __page_shift;
}
