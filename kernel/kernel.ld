ENTRY(kentry)
SECTIONS
{
    /*
     * Compress everything down into the .text section.
     * That way, `objcopy -Obinary' will include everything we need.
     *
     * THIS IS NEEDED BECAUSE THE CURRENT KERNEL LOADER DOES NOT KNOW WHERE TO
     * PLACE NON-CODE SECTIONS, AND THUS DOES NOT OPEN UP THE NECESSARY PAGES
     * FOR NON-CODE, LEADING TO PAGE FAULTS. As a quick fix, just stuff
     * everything into the .text section. This will result in a bloated kernel
     * image, but that is fine for right now as we have plenty of floppy disk
     * space :-)
     */
    .text 0x20000 :         /* TODO: can we include base address from config.h? */
    {
        *entry.o(.text)     /* entry.o goes first so kentry first instruction */
        *(.text)
        *(.data)
        *(.bss)
        *(.rodata*)
    }
}
