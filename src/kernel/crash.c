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
 *         File: kernel/crash.c
 *      Created: February 13, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdarg.h>
#include <stdio.h>

#include <ohwes.h>
#include <console.h>
#include <interrupt.h>
#include <irq.h>
#include <x86.h>
#include <cpu.h>

#define CRASH_COLOR     CONSOLE_BLUE
#define CRASH_BANNER    " OH-WES "
#define CRASH_WIDTH     80

const char * ExceptionNames[] =
{
    /*0x00*/ "DIVIDE_ERROR",
    /*0x01*/ "DEBUG_EXCEPTION",
    /*0x02*/ "NON_MASKABLE_INTERRUPT",
    /*0x03*/ "BREAKPOINT",
    /*0x04*/ "OVERFLOW",
    /*0x05*/ "BOUND_RANGE_EXCEEDED",
    /*0x06*/ "INVALID_OPCODE",
    /*0x07*/ "DEVICE_NOT_AVAILABLE",
    /*0x08*/ "DOUBLE_FAULT",
    /*0x09*/ "EXCEPTION_09",
    /*0x0A*/ "INVALID_TSS",
    /*0x0B*/ "SEGMENT_NOT_PRESENT",
    /*0x0C*/ "STACK_FAULT",
    /*0x0D*/ "GENERAL_PROTECTION_FAULT",
    /*0x0E*/ "PAGE_FAULT",
    /*0x0F*/ "EXCEPTION_0F",
    /*0x10*/ "MATH_FAULT",
    /*0x11*/ "ALIGNMENT_CHECK",
    /*0x12*/ "MACHINE_CHECK",
    /*0x13*/ "SIMD_FLOATING_POINT_EXCEPTION",
    /*0x14*/ "VIRTUALIZATION_EXCEPTION",
    /*0x15*/ "CONTROL_PROTECTION_EXCEPTION",
    /*0x16*/ "EXCEPTION_16",
    /*0x17*/ "EXCEPTION_17",
    /*0x18*/ "EXCEPTION_18",
    /*0x19*/ "EXCEPTION_19",
    /*0x1A*/ "EXCEPTION_1A",
    /*0x1B*/ "EXCEPTION_1B",
    /*0x1C*/ "EXCEPTION_1C",
    /*0x1D*/ "EXCEPTION_1D",
    /*0x1E*/ "EXCEPTION_1E",
    /*0x1F*/ "EXCEPTION_1F",
};
static_assert(countof(ExceptionNames) == NUM_EXCEPTIONS, "Bad ExceptionNames length");

void center_text(const char *str, ...)
{
    va_list args;
    char buf[CRASH_WIDTH];

    va_start(args, str);
    vsnprintf(buf, sizeof(buf), str, args);
    va_end(args);

    int len = strlen(buf);
    int col = (CRASH_WIDTH - len) / 2;

    if (col < 0) {
        col = 0;
    }
    printf("\e[%dG%s", col, buf);
}

void print_segsel(int segsel)
{
    struct segsel *ss = (struct segsel *) &segsel;
    struct x86_desc *desc = get_seg_desc(segsel);
    printf("%04X(%04X|%d|%d) %08X,%X", segsel,
        ss->index, ss->ti, ss->rpl,
        desc->seg.basehi << 24 | desc->seg.baselo,
        desc->seg.limithi << 16 | desc->seg.limitlo);
}

void print_flags(uint32_t eflags)
{
    struct eflags *flags = (struct eflags *) &eflags;
    printf("EFL=%08X IOPL=%d", eflags, flags->iopl);
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

__fastcall __noreturn
void crash(struct iregs *regs)
{
    irq_setmask(0xFFFF);
    irq_unmask(IRQ_KEYBOARD);   // leave only keyboard interrupt
    sti();                      // allow CTRL+ALT+DEL

    uint16_t _cs; store_cs(_cs);
    struct segsel *curr_cs = (struct segsel *) &_cs;
    struct segsel *fault_cs = (struct segsel *) &regs->cs;

    int curr_pl = curr_cs->rpl;
    int fault_pl = fault_cs->rpl;

    bool pl_change = false;
    if (curr_pl != fault_pl) {
        pl_change = true;
    }

    printf("\e[0;0H\e[37;4%dm\e[2J\e5", CRASH_COLOR);   // cursor top left, set color, clear screen, hide cursor
    printf("\n\n\n\n");
    printf("\e[47;3%dm", CRASH_COLOR);                  // set banner color
    center_text(CRASH_BANNER);
    printf("\e[37;4%dm", CRASH_COLOR);                  // clear banner color
    printf("\n\n");
    center_text("A fatal exception %02X has occurred at %04X:%08X.",
        regs->vec_num, regs->cs, regs->eip);
    printf("\n\n");
    center_text("%s", ExceptionNames[regs->vec_num]);
    printf("\n\n\n");

    printf("\nEAX=%08X EBX=%08X\nECX=%08X EDX=%08X",
        regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("\nEDI=%08X ESI=%08X\nEBP=%08X %s=%08X",
        regs->edi, regs->esi, regs->ebp,
        (pl_change) ? "ESP" : "EIP",
        (pl_change) ? regs->esp : regs->eip);
    if (pl_change) {
        printf("\nEIP=%08X", regs->eip);
        printf(" ERR=%08X", regs->err_code);
        printf("\n");
        print_flags(regs->eflags);
    }
    else {
        printf("\n");
        print_flags(regs->eflags);
        if (regs->err_code) {
            printf("\nERR=%08X", regs->err_code);
        }
    }
    printf("\n");
    printf("\nCS="); print_segsel(regs->cs);
    printf("\nDS="); print_segsel(regs->ds);
    printf("\nES="); print_segsel(regs->es);
    printf("\nFS="); print_segsel(regs->fs);
    printf("\nGS="); print_segsel(regs->gs);
    if (pl_change) {
        printf("\nSS="); print_segsel(regs->ss & 0xFFFF);
    }

    die();
}

__noreturn
void kpanic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    kprint("panic: ");
    vprintf(fmt, args);

    va_end(args);
    die();
}
