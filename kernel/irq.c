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
#include <interrupt.h>
#include <irq.h>
#include <ohwes.h>
#include <pic.h>

extern __fastcall void crash(struct iregs *regs);

#define valid_irq(n)    ((n) >= 0 && (n) < NUM_IRQS)

#define MAX_HANDLERS    8

static irq_handler handler_map[NUM_IRQS][MAX_HANDLERS] = { };

void init_irq(void)
{
    zeromem(handler_map, sizeof(handler_map));
}

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
    assert(valid_irq(irq_num));

    bool registered = false;
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handler_map[irq_num][i] == NULL) {
            handler_map[irq_num][i] = func;
            registered = true;
            break;
        }
    }

    if (!registered) {
        panic("maximum number of handlers already registered for IRQ %d", irq_num);
    }
}

void irq_unregister(int irq_num, irq_handler func)
{
    assert(valid_irq(irq_num));

    bool unregistered = false;
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handler_map[irq_num][i] == func) {
            handler_map[irq_num][i] = NULL;
            unregistered = true;
            break;
        }
    }

    if (!unregistered) {
        panic("handler %08X not registered for IRQ %d", func, irq_num);
    }
}

void __fastcall handle_irq(struct iregs *regs)
{
    int irq_num;
    irq_handler handler;
    bool handled = false;

    irq_num = ~regs->vec_num;
    if (!valid_irq(irq_num)) {
        panic("unknown device IRQ number: %d", irq_num);
    }

    handled = false;
    for (int i = 0; i < MAX_HANDLERS; i++) {
        handler = handler_map[irq_num][i];
        if (handler != NULL) {
            handler();
            handled = true;
        }
    }

    if (!handled) {
        crash(regs);
    }

    pic_eoi(irq_num);
}
