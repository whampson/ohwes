ENTRY(_start)
SECTIONS
{
    .text 0x80000 :
    {
        *entry.o(.text)
        *(.text)
        *(.data)
        *(.bss)
        *(.rodata*)
    }
}
