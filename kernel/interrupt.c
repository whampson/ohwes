/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
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

#include <nb/init.h>
#include <nb/interrupt.h>
#include <nb/irq.h>
#include <nb/exception.h>
#include <nb/kernel.h>
#include <x86/desc.h>

#define THUNK_PROTOTYPE(thunk_name) \
extern void thunk_name(void);

THUNK_PROTOTYPE(thunk_syscall)

#define set_segdesc_except(desc,handler)                            \
do {                                                                \
    desc->trap_gate.type = SEGDESC_TYPE_TRAP32;                     \
    desc->trap_gate.segsel = KERNEL_CS;                             \
    desc->trap_gate.dpl = KERNEL_PL;                                \
    desc->trap_gate.p = handler != NULL;                            \
    desc->trap_gate.offset_lo = ((uint32_t) handler) & 0xFFFF;      \
    desc->trap_gate.offset_hi = ((uint32_t) handler) >> 16;         \
} while (0)

#define set_segdesc_irq(desc,handler)                               \
do {                                                                \
    desc->int_gate.type = SEGDESC_TYPE_INT32;                       \
    desc->int_gate.segsel = KERNEL_CS;                              \
    desc->int_gate.dpl = KERNEL_PL;                                 \
    desc->int_gate.p = handler != NULL;                             \
    desc->int_gate.offset_lo = ((uint32_t) handler) & 0xFFFF;       \
    desc->int_gate.offset_hi = ((uint32_t) handler) >> 16;          \
} while (0)

#define set_segdesc_syscall(desc,handler)                           \
do {                                                                \
    desc->trap_gate.type = SEGDESC_TYPE_TRAP32;                     \
    desc->trap_gate.segsel = USER_CS;                               \
    desc->trap_gate.dpl = USER_PL;                                  \
    desc->trap_gate.p = handler != NULL;                            \
    desc->trap_gate.offset_lo = ((uint32_t) handler) & 0xFFFF;      \
    desc->trap_gate.offset_hi = ((uint32_t) handler) >> 16;         \
} while (0)

typedef void (*handler_thunk)(void);
static const handler_thunk E_THUNKS[NUM_EXCEPT];
static const handler_thunk I_THUNKS[NUM_IRQ];

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
        if (i >= E_BASE_VECTOR && i + E_BASE_VECTOR < NUM_EXCEPT) {
            set_trap_desc(desc, KERNEL_CS, KERNEL_PL, E_THUNKS[i]);
        }
        if (i >= I_BASE_VECTOR && i + I_BASE_VECTOR < NUM_IRQ) {
            set_intr_desc(desc, KERNEL_CS, KERNEL_PL, I_THUNKS[i]);
        }
        if (i == SYSCALL_VECTOR) {
            set_trap_desc(desc, KERNEL_CS, USER_PL, thunk_syscall);
        }
    }

    struct descreg *idtr = (struct descreg *) IDT_REGPTR;
    idtr->base = IDT_BASE;
    idtr->limit = IDT_SIZE - 1;
    lidt(*idtr);
}

THUNK_PROTOTYPE(thunk_except_00)
THUNK_PROTOTYPE(thunk_except_01)
THUNK_PROTOTYPE(thunk_except_02)
THUNK_PROTOTYPE(thunk_except_03)
THUNK_PROTOTYPE(thunk_except_04)
THUNK_PROTOTYPE(thunk_except_05)
THUNK_PROTOTYPE(thunk_except_06)
THUNK_PROTOTYPE(thunk_except_07)
THUNK_PROTOTYPE(thunk_except_08)
THUNK_PROTOTYPE(thunk_except_09)
THUNK_PROTOTYPE(thunk_except_10)
THUNK_PROTOTYPE(thunk_except_11)
THUNK_PROTOTYPE(thunk_except_12)
THUNK_PROTOTYPE(thunk_except_13)
THUNK_PROTOTYPE(thunk_except_14)
THUNK_PROTOTYPE(thunk_except_15)
THUNK_PROTOTYPE(thunk_except_16)
THUNK_PROTOTYPE(thunk_except_17)
THUNK_PROTOTYPE(thunk_except_18)
THUNK_PROTOTYPE(thunk_except_19)
THUNK_PROTOTYPE(thunk_except_20)
THUNK_PROTOTYPE(thunk_except_21)
THUNK_PROTOTYPE(thunk_except_22)
THUNK_PROTOTYPE(thunk_except_23)
THUNK_PROTOTYPE(thunk_except_24)
THUNK_PROTOTYPE(thunk_except_25)
THUNK_PROTOTYPE(thunk_except_26)
THUNK_PROTOTYPE(thunk_except_27)
THUNK_PROTOTYPE(thunk_except_28)
THUNK_PROTOTYPE(thunk_except_29)
THUNK_PROTOTYPE(thunk_except_30)
THUNK_PROTOTYPE(thunk_except_31)
THUNK_PROTOTYPE(thunk_irq_00)
THUNK_PROTOTYPE(thunk_irq_01)
THUNK_PROTOTYPE(thunk_irq_02)
THUNK_PROTOTYPE(thunk_irq_03)
THUNK_PROTOTYPE(thunk_irq_04)
THUNK_PROTOTYPE(thunk_irq_05)
THUNK_PROTOTYPE(thunk_irq_06)
THUNK_PROTOTYPE(thunk_irq_07)
THUNK_PROTOTYPE(thunk_irq_08)
THUNK_PROTOTYPE(thunk_irq_09)
THUNK_PROTOTYPE(thunk_irq_10)
THUNK_PROTOTYPE(thunk_irq_11)
THUNK_PROTOTYPE(thunk_irq_12)
THUNK_PROTOTYPE(thunk_irq_13)
THUNK_PROTOTYPE(thunk_irq_14)
THUNK_PROTOTYPE(thunk_irq_15)

static const handler_thunk E_THUNKS[NUM_EXCEPT] =
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

static const handler_thunk I_THUNKS[NUM_IRQ] =
{
    thunk_irq_00,       thunk_irq_01,       thunk_irq_02,       thunk_irq_03,
    thunk_irq_04,       thunk_irq_05,       thunk_irq_06,       thunk_irq_07,
    thunk_irq_08,       thunk_irq_09,       thunk_irq_10,       thunk_irq_11,
    thunk_irq_12,       thunk_irq_13,       thunk_irq_14,       thunk_irq_15
};
