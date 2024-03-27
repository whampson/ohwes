#ifndef __IRQ_H
#define __IRQ_H

/**
 * Device IRQ numbers.
 * NOTE: these are NOT interrupt vector numbers!
 */
#define IRQ_TIMER           0       // Programmable Interval Timer (PIT)
#define IRQ_KEYBOARD        1       // PS/2 Keyboard
#define IRQ_SLAVE           2       // Slave PIC cascade signal
#define IRQ_COM2            3       // Serial Port #2
#define IRQ_COM1            4       // Serial Port #1
#define IRQ_LPT2            5       // Parallel Port #2
#define IRQ_FLOPPY          6       // Floppy Disk Controller
#define IRQ_LPT1            7       // Parallel Port #1
#define IRQ_RTC             8       // Real-Time Clock (RTC)
#define IRQ_ACPI            9       // ACPI Control Interrupt
#define IRQ_MISC1           10      // (possibly scsi or nic)
#define IRQ_MISC2           11      // (possibly scsi or nic)
#define IRQ_MOUSE           12      // PS/2 Mouse
#define IRQ_COPOCESSOR      13      // Coprocessor Interrupt
#define IRQ_ATA1            14      // ATA Channel #1
#define IRQ_ATA2            15      // ATA Channel #2
#define NUM_IRQS            16

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <stdint.h>

typedef void (*irq_handler)(void);

void irq_mask(int irq_num);
void irq_unmask(int irq_num);

uint16_t irq_getmask(void);
void irq_setmask(uint16_t mask);

void irq_register(int irq_num, irq_handler func);
void irq_unregister(int irq_num, irq_handler func);

#endif // __ASSEMBLER__

#endif // __IRQ_H
