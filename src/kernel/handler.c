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
#include <console.h>
#include <interrupt.h>
#include <x86.h>
#include <cpu.h>

#define FANCY_CRASH     1
#define CRASH_COLOR     CONSOLE_BLUE
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
    char buf[256];
    printf("\e[0;0H\e[37;4%dm\e[2J\e5", CRASH_COLOR);   // cursor top left, set color, clear screen, hide cursor
    printf("\n\n\n\n\n\n");
    printf("\e[47;3%dm", CRASH_COLOR);    // set banner color
    fancy_crash_center_text(CRASH_BANNER);
    printf("\e[37;4%dm", CRASH_COLOR);                  // clear banner color
    printf("\n\n");
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
    printf("\n\n\n\n");
#else
    panic("\nfatal exception %02x in ring %d at %08x!", regs->vec_num, fault_pl, regs->eip);
#endif

    printf("\nEAX=%08x EBX=%08x\nECX=%08x EDX=%08x",
        regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("\nESI=%08x EDI=%08x\nEBP=%08x %s=%08x",
        regs->esi, regs->edi, regs->ebp,
        (pl_change) ? "ESP" : "EIP",
        (pl_change) ? regs->esp : regs->eip);
    if (pl_change) {
        printf("\nEIP=%08x", regs->eip);
    }
    printf("\nCS="); print_segsel(regs->cs);
    printf("\nDS="); print_segsel(regs->ds);
    printf("\nES="); print_segsel(regs->es);
    printf("\nFS="); print_segsel(regs->fs);
    printf("\nGS="); print_segsel(regs->gs);
    if (pl_change) {
        printf("\nSS="); print_segsel(regs->ss & 0xFFFF);
    }
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
int recv_syscall(struct iregs *regs)
{
    assert(regs->vec_num == INT_SYSCALL);

    uint32_t func = regs->eax;
    int status = -1;

    switch (func) {
        case SYS_EXIT:
            status = sys_exit(regs->ebx);
            break;
        default:
            panic("unexpected syscall %d at %08x\n", func, regs->eip);
    }

    return status;
}
