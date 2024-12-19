ENTRY(_start)
SECTIONS
{
    .text 0x60000 :
    {
        *_start.o(.text)
        *(.text)
        *(.data)
        *(.bss)
        *(.rodata*)
    }
}
