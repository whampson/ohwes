#ifndef __NIOBIUM_H
#define __NIOBIUM_H

/* Segment Selectors */
#define KERNEL_CS               0x10    /* Kernel Code Segment */
#define KERNEL_DS               0x18    /* Kernel Data Segment */
#define USER_CS                 0x23    /* User-space Code Segment */
#define USER_DS                 0x2B    /* User-space Data Segment */
#define TSS_SEG                 0x30    /* TSS Segment */
#define LDT_SEG                 0x38    /* LDT Segment */

void gdt_init(void);
void ldt_init(void);
void idt_init(void);
void tss_init(void);

#endif /* __NIOBIUM_H */
