/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
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
#include <irq.h>
#include <ohwes.h>
#include <pic.h>

#define MAX_ISR 8

#define IRQ_VALID(n) ((n) >= 0 && (n) < NR_IRQS)

irq_handler isr_map[NR_IRQS][MAX_ISR];

__fastcall void crash(struct iregs *regs); // crash.c

void irq_mask(int irq_num)
{
    pic_mask(irq_num);
}

void irq_unmask(int irq_num)
{
    pic_unmask(irq_num);
}

uint16_t irq_getmask(void)
{
    return pic_getmask();
}

void irq_setmask(uint16_t mask)
{
    pic_setmask(mask);
}

void irq_register(int irq_num, irq_handler func)
{
    assert(IRQ_VALID(irq_num));

    bool registered = false;
    for (int i = 0; i < MAX_ISR; i++) {
        if (isr_map[irq_num][i] == NULL) {
            isr_map[irq_num][i] = func;
            registered = true;
            break;
        }
    }

    if (!registered) {
        kprint("irq: maximum number of handlers already registered for IRQ %d\n", irq_num);    // kwarn?
    }
}

void irq_unregister(int irq_num, irq_handler func)
{
    assert(IRQ_VALID(irq_num));

    bool unregistered = false;
    for (int i = 0; i < MAX_ISR; i++) {
        if (isr_map[irq_num][i] == func) {
            isr_map[irq_num][i] = NULL;
            unregistered = true;
            break;
        }
    }

    if (!unregistered) {
        kprint("irq: handler %08X not registered for IRQ %d\n", func, irq_num);
    }
}

__fastcall void handle_irq(struct iregs *regs)
{
    int irq_num;
    irq_handler handler;
    bool handled = false;

    irq_num = ~regs->vec_num;
    pic_eoi(irq_num);

    handled = false;
    for (int i = 0; i < MAX_ISR; i++) {
        handler = isr_map[irq_num][i];
        if (handler != NULL) {
            handler(irq_num);
            handled = true;
        }
    }

    if (!handled) {
        crash(regs);
    }
}
