
/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: kernel/irq.c                                                      *
 * Created: December 23, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <stddef.h>
#include <ohwes/kernel.h>
#include <ohwes/interrupt.h>
#include <ohwes/irq.h>
#include <drivers/pic.h>

#define PIC_MASTER  0
#define PIC_SLAVE   1

#define SLAVE_IRQ   2           /* IRQ line of slave PIC on master */

/* Initialization Command Words */
#define ICW1        0x11            /* edge-triggered, 8 lines, cascade mode, ICW4 needed */
#define ICW2_M      (INT_IRQ)       /* master PIC base interrupt vector */
#define ICW2_S      (INT_IRQ+8)     /* slave PIC base interrupt vector */
#define ICW3_M      (1<<(SLAVE_IRQ))/* mask of slave IRQ on master */
#define ICW3_S      (SLAVE_IRQ)     /* slave IRQ number, to be sent to master */
#define ICW4        0x01            /* not fully nested, not auto EOI, 8086 mode */

/* End of Interrupt Command */
#define EOI         0x60    /* specific EOI, requires IRQ num in low bits */

/* Interrupt Masks */
#define MASK_ALL    0xFF    /* Mask all interrupts */

#define valid_irq(n)    ((n) >= 0 && (n) < NUM_IRQ)

static irq_handler handler_map[NUM_IRQ] = { 0 };  /* TODO: linked list */

void irq_init(void)
{
    /* configure master PIC */
    i8259_cmd_write(PIC_MASTER, ICW1);
    i8259_data_write(PIC_MASTER, ICW2_M);
    i8259_data_write(PIC_MASTER, ICW3_M);
    i8259_data_write(PIC_MASTER, ICW4);

    /* configure slave PIC */
    i8259_cmd_write(PIC_SLAVE, ICW1);
    i8259_data_write(PIC_SLAVE, ICW2_S);
    i8259_data_write(PIC_SLAVE, ICW3_S);
    i8259_data_write(PIC_SLAVE, ICW4);

    /* mask all IRQs, except slave line on master PIC */
    i8259_data_write(PIC_MASTER, MASK_ALL & ~(1<<SLAVE_IRQ));
    i8259_data_write(PIC_SLAVE, MASK_ALL);
}

void irq_mask(int irq_num)
{
    int pic_num = (irq_num >= 8);
    int mask = (1 << (irq_num & 7));

    uint8_t oldmask = i8259_data_read(pic_num);
    i8259_data_write(pic_num, oldmask | mask);
}

void irq_unmask(int irq_num)
{
    int pic_num = (irq_num >= 8);
    int mask = (1 << (irq_num & 7));

    uint8_t oldmask = i8259_data_read(pic_num);
    i8259_data_write(pic_num, oldmask & ~mask);
}

void irq_eoi(int irq_num)
{
    if (irq_num >= 8) {
        i8259_cmd_write(PIC_SLAVE, EOI | (irq_num & 7));
    }
    i8259_cmd_write(PIC_MASTER, EOI | (irq_num & 7));
}

bool irq_register_handler(int irq_num, irq_handler func)
{
    if (!valid_irq(irq_num)) {
        return false;
    }
    if (handler_map[irq_num] != NULL) {
        return false;
    }

    handler_map[irq_num] = func;
    return true;
}

void irq_unregister_handler(int irq_num)
{
    if (!valid_irq(irq_num)) {
        return;
    }

    handler_map[irq_num] = NULL;
}

__fastcall void handle_irq(struct iframe *regs)
{
    int irq_num = ~regs->vec_num;

    if (!valid_irq(irq_num)) {
        panic("Unexpected IRQ number: %d", irq_num);
    }

    irq_handler handler = handler_map[irq_num];
    if (handler != NULL) {
        handler();
    }
    else {
        kprintf("Unhandled IRQ %d!\n", irq_num);
    }

    irq_eoi(irq_num);   /* TODO: pass EOI responsibility onto handler? */
}
