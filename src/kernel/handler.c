/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *         File: kernel/handler.c
 *      Created: April 4, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdarg.h>
#include <stdio.h>
#include <ohwes.h>
#include <interrupt.h>

__fastcall
void recv_interrupt(struct iregs *regs)
{
    if (regs->vec_num >= INT_EXCEPTION &&
        regs->vec_num < INT_EXCEPTION + NUM_EXCEPTION)
    {
        panic("exception %02x occurred at %#08x! (error code = %d)", regs->vec_num, regs->eip, regs->err_code);
    }

    panic("unexpected interrupt %d at %#08x!", regs->eax, regs->eip);
}


__fastcall
void recv_syscall(struct iregs *regs)
{
    printf("eip = %08x, esp = %08x, ss = %2x\n", regs->eip, regs->esp, regs->ss);
    printf("unexpected system call %d at %#08x!\n", regs->eax, regs->eip);
}
