#ifndef __INIT_H
#define __INIT_H

/**
 * Interrupt Descriptor Table
 */
#define IDT_COUNT           256
#define IDT_BASE            0x1000
#define IDT_LIMIT           (IDT_BASE+(IDT_COUNT*DESC_SIZE-1))
#define IDT_SIZE            (IDT_LIMIT+1)


/**
 * Global Descriptor Table
 */
#define GDT_COUNT           8
#define GDT_BASE            0x1800
#define GDT_LIMIT           (GDT_BASE+(GDT_COUNT*DESC_SIZE-1))
#define GDT_SIZE            (GDT_LIMIT+1)


/**
 * Local Descriptor Table
 */
#define LDT_COUNT           2
#define LDT_BASE            0x1840
#define LDT_LIMIT           (LDT_BASE+(LDT_COUNT*DESC_SIZE-1))
#define LDT_SIZE            (LDT_LIMIT+1)


/**
 * Task State Segment
 */
#define TSS_BASE            0x1880
#define TSS_LIMIT           (TSS_BASE+TSS_SIZE-1)


#endif  // __INIT_H
