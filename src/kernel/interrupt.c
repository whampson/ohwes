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

#include <stdarg.h>
#include <stdio.h>
#include <ohwes.h>
#include <interrupt.h>
#include <x86.h>

__fastcall
void dumpregs(struct iregs *regs)
{
    struct eflags *flags = (struct eflags *) &regs->eflags;

    uint16_t cs;
    store_cs(cs);

    printf("VEC_NUM=%d, ERR_CODE=%d\n",
        regs->vec_num, regs->err_code);
    printf("EAX=%08x  EBX=%08x  ECX=%08x  EDX=%08xh\n",
        regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("ESI=%08x  EDI=%08x  EBP=%08x  EIP=%08xh\n",
        regs->esi, regs->edi, regs->ebp, regs->eip);
    printf("EFLAGS=%08x [ IOPL=%d", regs->eflags, flags->iopl);
    {
        if (flags->id)   printf(" ID");
        if (flags->vip)  printf(" VIP");
        if (flags->vif)  printf(" VIF");
        if (flags->ac)   printf(" AC");
        if (flags->vm)   printf(" VM");
        if (flags->rf)   printf(" RF");
        if (flags->nt)   printf(" NT");
        if (flags->of)   printf(" OF");
        if (flags->df)   printf(" DF");
        if (flags->intf) printf(" IF");
        if (flags->tf)   printf(" TF");
        if (flags->sf)   printf(" SF");
        if (flags->zf)   printf(" ZF");
        if (flags->af)   printf(" AF");
        if (flags->pf)   printf(" PF");
        if (flags->cf)   printf(" CF");
    }
    printf(" ]\n");
    printf("CS=%04x\n", regs->cs);
    printf("DS=%04x\n", regs->ds);
    printf("ES=%04x\n", regs->es);
    printf("FS=%04x\n", regs->fs);
    printf("GS=%04x\n", regs->gs);


    struct segsel *cs_segsel_curr = (struct segsel *) &cs;
    struct segsel *cs_segsel_fault = (struct segsel *) &regs->cs;

    if (cs_segsel_curr->rpl != cs_segsel_fault->rpl) {
        printf("PRIVILEGE LEVEL CHANGED FROM %d to %d\n",
            cs_segsel_fault->rpl, cs_segsel_curr->rpl);
        printf("SS=%04x\n", regs->ss);
        printf("ESP=%08x\n", regs->esp);
    }
    else {
        printf("NO PRIVILEGE LEVEL CHANGE\n");
    }
}

__fastcall __noreturn
void exception(struct iregs *regs)
{
    dumpregs(regs);
    for (;;);
}

__fastcall
void recv_interrupt(struct iregs *regs)
{
    if (regs->vec_num >= INT_EXCEPTION &&
        regs->vec_num < INT_EXCEPTION + NUM_EXCEPTION)
    {
        exception(regs);
    }

    panic("got unexpected interrupt %d at %#08x!", regs->eax, regs->eip);
}
