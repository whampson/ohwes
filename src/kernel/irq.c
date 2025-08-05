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
 *         File: kernel/irq.c
 *      Created: December 23, 2020
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stddef.h>
#include <stdio.h>
#include <i386/bitops.h>
#include <i386/pic.h>
#include <i386/interrupt.h>
#include <i386/x86.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>

#define MAX_ISR             8   // max ISR handlers per IRQ line
#define SPURIOUS_THRESH     10

#define _IRQ_VALID(n)        ((n) >= 0 && (n) < NR_IRQS)
#define _IRQ_MASKED(n)       (irq_getmask() & (1 << (n)))

struct irq_stats {
    int spur_pic0;
    int spur_pic1;
};

static struct irq_stats _irqstats;
struct irq_stats *g_irqstats = &_irqstats;

static irq_handler _isr_map[NR_IRQS][MAX_ISR];

void irq_enable(void)
{
    __sti();
}

void irq_disable(void)
{
    __cli();
}

void irq_mask(int irq)
{
    pic_mask(irq);
}

void irq_unmask(int irq)
{
    pic_unmask(irq);
}

uint16_t irq_getmask(void)
{
    return pic_getmask();
}

void irq_setmask(uint16_t mask)
{
    pic_setmask(mask);
}

void irq_register(int irq, irq_handler func)
{
    assert(_IRQ_VALID(irq));

    bool registered = false;
    for (int i = 0; i < MAX_ISR; i++) {
        if (_isr_map[irq][i] == NULL) {
            _isr_map[irq][i] = func;
            registered = true;
            break;
        }
    }

    if (!registered) {
        panic("maximum number of handlers already registered for IRQ %d", irq);
    }
}

void irq_unregister(int irq, irq_handler func)
{
    assert(_IRQ_VALID(irq));

    bool unregistered = false;
    for (int i = 0; i < MAX_ISR; i++) {
        if (_isr_map[irq][i] == func) {
            _isr_map[irq][i] = NULL;
            unregistered = true;
            break;
        }
    }

    if (!unregistered) {
        panic("handler at 0x%08tX not registered for IRQ %d", (intptr_t) func, irq);
    }
}
__fastcall void handle_irq(struct iregs *regs)
{
    int irq = ~regs->vec;
    bool handled = false;
    bool masked = _IRQ_MASKED(irq);

    if ((irq == 7 && masked) || (irq == 15 && masked)) {
        bool pic0 = irq == 7;
        int count = (pic0)
            ? (++g_irqstats->spur_pic0)
            : (++g_irqstats->spur_pic1);

        alert("spurious IRQ%d\n", irq);
        if (count > 1 && ((count-1) % SPURIOUS_THRESH) == 0) {
            alert("more than %d spurious IRQs! what's going on??\n", count - 1);
        }
        return;     // no EOI for spurious IRQs
    }

    pic_eoi(irq);

    if (!masked) {
        for (int i = 0; i < MAX_ISR; i++) {
            irq_handler isr = _isr_map[irq][i];
            if (isr != NULL) {
                isr(irq, regs);
                handled = true;
            }
        }
    }

    if (!handled) {
        alert("unhandled irq%d\n", irq);
    }
}
