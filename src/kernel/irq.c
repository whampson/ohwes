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

#define valid_irq(n)    ((n) >= 0 && (n) < NUM_IRQ)

static irq_handler handler_map[NUM_IRQ] = { };

void init_irq(void)
{
    memset(handler_map, 0, sizeof(handler_map));
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

bool irq_register(int irq_num, irq_handler func)
{
    if (!valid_irq(irq_num)) {
        return false;
    }
    if (handler_map[irq_num] != NULL) {
        panic("irq %d handler already registered at %08X", irq_num, handler_map[irq_num]);
    }

    handler_map[irq_num] = func;
    return true;
}

void irq_unregister(int irq_num)
{
    if (!valid_irq(irq_num)) {
        return;
    }

    handler_map[irq_num] = NULL;
}

__fastcall
void recv_irq(struct iregs *regs)
{
    int irq_num = ~regs->vec_num;

    if (!valid_irq(irq_num)) {
        panic("unknown device IRQ number: %d", irq_num);
    }

    irq_handler handler = handler_map[irq_num];
    if (handler != NULL) {
        handler();
    }
    else {
        panic("unhandled IRQ %d", irq_num);
    }

    pic_eoi(irq_num);
}
