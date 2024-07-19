OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

SECTIONS
{
    /*
     * Compress everything down into the .text section.
     * That way, `objcopy -Obinary' will include everything we need.
     */
    .text 0x10000 :
    {
        *(.setup)
        *(.text)
        *(.rodata*)
        *(.data)
        *(.bss)
    }
}
