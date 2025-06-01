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
 *         File: i386/kernel/interrupt.c
 *      Created: April 24, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <i386/cpu.h>
#include <i386/gdbstub.h>
#include <i386/io.h>
#include <i386/x86.h>
#include <kernel/ohwes.h>
#include <kernel/kernel.h>

uint32_t get_esp(const struct iregs *regs)
{
    uint32_t esp = (get_rpl(regs) != get_cpl())
        // if we changed privilege levels, ESP in iregs is valid
        ? (uint32_t) regs->esp
        // if we did not change privilege levels, no ESP (or SS) was pushed to
        // the stack and the stack did not switch, so to find the top of the
        // faulting stack we must subtract (or add, because Intel) the size of
        // the iregs structure minus the ESP and SS values from the top of the
        // interrupt stack
        : (uint32_t) (((char *) regs) + SIZEOF_IREGS_NO_PL_CHANGE);

    return esp;
}

uint16_t get_ss(const struct iregs *regs)
{
    uint16_t curr_ss;
    store_ss(curr_ss);

    return (get_rpl(regs) != get_cpl())
        ? regs->ss
        : curr_ss;
}

int get_rpl(const struct iregs *regs)
{
    return ((struct segsel *) &regs->cs)->rpl;
}
