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
#include <cpu.h>

#define FANCY_CRASH     1
#define CRASH_COLOR     4       // blue
#define CRASH_BANNER    " OH-WES "

void fancy_crash_center_text(const char *str)
{
    int len = strlen(str);
    int col = (80 - len) / 2;

    if (col < 0) {
        col = 0;
    }
    printf("\e[%dG%s", col, str);
}

void print_segsel(int segsel)
{
    struct segsel *ss = (struct segsel *) &segsel;
    struct x86_desc *desc = get_seg_desc(segsel);
    printf("%04x(%04x|%d|%d) %08x %08x", segsel,
        ss->index, ss->ti, ss->rpl,
        desc->seg.basehi << 24 | desc->seg.baselo,
        desc->seg.limithi << 16 | desc->seg.limitlo);
}

__fastcall __noreturn
void exception(struct iregs *regs)
{
    uint16_t _cs; store_cs(_cs);
    struct segsel *curr_cs = (struct segsel *) &_cs;
    struct segsel *fault_cs = (struct segsel *) &regs->cs;
    struct eflags *flags = (struct eflags *) &regs->eflags;

    int curr_pl = curr_cs->rpl;
    int fault_pl = fault_cs->rpl;

    bool pl_change = false;
    if (curr_pl != fault_pl) {
        pl_change = true;
    }

#if FANCY_CRASH

#define BG 40
#define FG 30

    char buf[256];

    printf("\e[0;0H\e[37;%dm\e[2J", BG|CRASH_COLOR);
    printf("\e[47;%dm\e[5B", FG|CRASH_COLOR);
    fancy_crash_center_text(CRASH_BANNER);
    printf("\e[37;%dm\n", BG|CRASH_COLOR);
    printf("\n");
    snprintf(
        buf, sizeof(buf),
        "A fatal exception %02x has occurred in ring %d at %08x.",
        regs->vec_num, fault_pl, regs->eip);
    fancy_crash_center_text(buf);
    printf("\n");

    if (regs->err_code) {
        snprintf(
            buf, sizeof(buf),
            "Error code: %08x",
            regs->err_code);
        fancy_crash_center_text(buf);
        printf("\n");
    }

    // TODO: I think this should retain the current column...:
    //      printf("\e[;5H");

    int row = 17;
    if (pl_change) {
        row--;
    }
    printf("\e[%d;0H", row);
    printf("\e5");

#endif
    printf("\nEAX=%08x EBX=%08x ECX=%08x EDX=%08x",
        regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("\nESI=%08x EDI=%08x EBP=%08x %s=%08x",
        regs->esi, regs->edi, regs->ebp,
        (pl_change) ? "ESP" : "EIP",
        (pl_change) ? regs->esp : regs->eip);
    {
        // printf("EFLAGS=%08x [", regs->eflags);
        printf("\nIOPL=%d", flags->iopl);
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
        // printf(" ]\n");
    }

    printf("\nCS="); print_segsel(regs->cs);
    printf("\nDS="); print_segsel(regs->ds);
    printf("\nES="); print_segsel(regs->es);
    printf("\nFS="); print_segsel(regs->fs);
    printf("\nGS="); print_segsel(regs->gs);
    if (pl_change) {
        printf("\nSS="); print_segsel(regs->ss & 0xFFFF);
    }

#if !FANCY_CRASH
    panic("\nfatal exception %02x in ring %d at %08x!", regs->vec_num, fault_pl, regs->eip);
#endif

    die();
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

__fastcall
void recv_syscall(struct iregs *regs)
{
    assert(regs->vec_num == INT_SYSCALL);

    uint32_t func = regs->eax;
    switch (func) {
        case SYS_EXIT:
            printf("back in the kernel... idling CPU\n");
            printf("user mode returned %d\n", regs->ebx);
            halt();
            break;
        default:
            panic("unexpected syscall %d at %08x\n", func, regs->eip);
    }
}
