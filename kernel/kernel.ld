OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

SECTIONS
{
    __setup_start = .;
    .setup 0x10000 :
    {
        *(.setup)
    }
    __setup_end = .;

    __kernel_start = .;
    .text 0xC0000000 + __kernel_start : AT (__kernel_start)
    {
        *(.text)
        *(.rodata*)
        *(.data)
        *(.bss)
    }
    __kernel_end = .;

    .eh_frame :
    {
        *(.eh_frame)
    }
}
