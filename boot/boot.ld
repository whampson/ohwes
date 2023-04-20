OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)

SECTIONS
{
    STAGE1_SEGMENT  = 0x07C0;
    STAGE1_ORIGIN   = STAGE1_SEGMENT << 4;

    STAGE2_SEGMENT  = 0x07E0;
    STAGE2_ORIGIN   = STAGE2_SEGMENT << 4;
    STAGE2_SECTOR   = 1;
    STAGE2_SIZE     = STAGE2_END - STAGE2_ORIGIN;
    STAGE2_COUNT    = STAGE2_SIZE / SECTOR_SIZE;

    KERNEL_SEGMENT  = 0x1000;
    KERNEL_SECTOR   = STAGE2_SECTOR + STAGE2_COUNT;
    KERNEL_COUNT    = 1;

    STACK_BASE      = STAGE1_ORIGIN;    /* x86 stack grows down */
    SECTOR_SIZE     = 512;

    .stage1 STAGE1_ORIGIN : AT(0)
    {
        *(.stage1)

        FILL(0xCC)      /* x86 nop */

        ASSERT(. <= SECTOR_SIZE - 2, "Boot sector is too large!");

        . = SECTOR_SIZE - 2;
        SHORT(0xAA55)   /* boot signature */
    }

    .stage2 STAGE2_ORIGIN : AT(STAGE2_SECTOR * SECTOR_SIZE)
    {
        ASSERT(STAGE2_ORIGIN == 0x7E00, "STAGE2_ORIGIN");
        *(.stage2)
    }

    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
    .rodata : { *(.rodata) }

    . = ALIGN(SECTOR_SIZE);
    STAGE2_END = .;

    kentry = .;
}
