#ifndef __IRQ_H
#define __IRQ_H

#include <stdbool.h>

typedef void (*irq_handler)(void);

void irq_mask(int irq_num);
void irq_unmask(int irq_um);
void irq_end(int irq_num);

bool irq_register(int irq_num, irq_handler func);
void irq_unregister(int irq_num);

#endif // __IRQ_H
