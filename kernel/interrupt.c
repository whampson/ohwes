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
 *    File: kernel/interrupt.c                                                *
 * Created: December 19, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <ohwes/init.h>
#include <ohwes/kernel.h>
#include <ohwes/except.h>
#include <ohwes/interrupt.h>
#include <ohwes/syscall.h>
#include <ohwes/irq.h>
#include <ohwes/thunk.h>
#include <x86/desc.h>

static const ivt_thunk thunk_except[NUM_EXCEPT];
static const ivt_thunk thunk_irq[NUM_IRQ];

void idt_init(void)
{
    struct x86_desc *idt;
    struct x86_desc *desc;
    int count;

    idt = (struct x86_desc *) IDT_BASE;
    memset(idt, 0, IDT_SIZE);

    count = IDT_SIZE / sizeof(struct x86_desc);
    for (int i = 0; i < count; i++) {
        desc = idt + i;
        if (i >= EXCEPT_BASE && i + EXCEPT_BASE < NUM_EXCEPT) {
            set_trap_desc(desc, KERNEL_CS, KERNEL_PL, thunk_except[i]);
        }
        if (i >= IRQ_BASE && i + IRQ_BASE < NUM_IRQ) {
            set_intr_desc(desc, KERNEL_CS, KERNEL_PL, thunk_irq[i]);
        }
        if (i == SYSCALL) {
            set_trap_desc(desc, KERNEL_CS, USER_PL, thunk_syscall);
        }
    }

    struct descreg *idtr = (struct descreg *) IDT_REGPTR;
    idtr->base = IDT_BASE;
    idtr->limit = IDT_SIZE - 1;
    lidt(*idtr);
}

static const ivt_thunk thunk_except[NUM_EXCEPT] =
{
    thunk_except_00,    thunk_except_01,    thunk_except_02,    thunk_except_03,
    thunk_except_04,    thunk_except_05,    thunk_except_06,    thunk_except_07,
    thunk_except_08,    thunk_except_09,    thunk_except_10,    thunk_except_11,
    thunk_except_12,    thunk_except_13,    thunk_except_14,    thunk_except_15,
    thunk_except_16,    thunk_except_17,    thunk_except_18,    thunk_except_19,
    thunk_except_20,    thunk_except_21,    thunk_except_22,    thunk_except_23,
    thunk_except_24,    thunk_except_25,    thunk_except_26,    thunk_except_27,
    thunk_except_28,    thunk_except_29,    thunk_except_30,    thunk_except_31
};

static const ivt_thunk thunk_irq[NUM_IRQ] =
{
    thunk_irq_00,       thunk_irq_01,       thunk_irq_02,       thunk_irq_03,
    thunk_irq_04,       thunk_irq_05,       thunk_irq_06,       thunk_irq_07,
    thunk_irq_08,       thunk_irq_09,       thunk_irq_10,       thunk_irq_11,
    thunk_irq_12,       thunk_irq_13,       thunk_irq_14,       thunk_irq_15
};
