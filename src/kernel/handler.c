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
 *         File: kernel/interrupt.c
 *      Created: January 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ohwes.h>

__fastcall __noreturn void crash(struct iregs *regs);

__fastcall
void exception(struct iregs *regs)
{
    crash(regs);
}

__fastcall
void recv_interrupt(struct iregs *regs)
{
    if (regs->vec_num >= IVT_EXCEPTION &&
        regs->vec_num < IVT_EXCEPTION + NUM_EXCEPTION)
    {
        exception(regs);
    }

    panic("got unexpected interrupt %d at %#08X!", regs->eax, regs->eip);
}

__fastcall
int recv_syscall(struct iregs *regs)
{
    assert(regs->vec_num == IVT_SYSCALL);

    uint32_t func = regs->eax;
    int status = -1;

    switch (func) {
        case SYS_EXIT:
            status = sys_exit(regs->ebx);
            break;
        default:
            panic("unexpected syscall %d at %08X\n", func, regs->eip);
    }

    return status;
}
