/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
 *
 * This file is part of the OH-WES Operating System.
 * OH-WES is free software; you may redistribute it and/or modify it under the
 * terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 *         File: src/include/kernel/irq.h
 *      Created: January 7, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

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
#define NR_IRQS             16

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <stdint.h>

#define IRQ_MASKALL         ((1<<NR_IRQS)-1)

typedef void (*irq_handler)(int irq, struct iregs *regs);

void irq_enable(void);
void irq_disable(void);

void irq_unmask(int irq);
void irq_mask(int irq);

uint16_t irq_getmask(void);
void irq_setmask(uint16_t mask);

void irq_register(int irq, irq_handler func);
void irq_unregister(int irq, irq_handler func);

#endif // __ASSEMBLER__

#endif // __IRQ_H
