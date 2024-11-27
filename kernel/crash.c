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

//
// NOTE: assumes console and keyboard are in working order!
//

#include <boot.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <ohwes.h>
#include <kernel.h>
#include <console.h>
#include <interrupt.h>
#include <irq.h>
#include <io.h>
#include <pic.h>
#include <x86.h>
#include <cpu.h>
#include <fs.h>
#include <paging.h>

#define CRASH_COLOR     CSI_BLUE
#define IRQ_COLOR       CSI_RED
#define NMI_COLOR       CSI_RED
#define BANNER_COLOR    CSI_BLUE
#define CRASH_BUFSIZ    1024

extern struct vga *g_vga;

static const char *exception_names[NR_EXCEPTIONS];

static void print_banner(const char *banner);
static void center_text(const char *str, ...);
static void set_background(int bg);

struct crash_screen {
    int bg; // background color
};

#define DECLARE_GLOBAL(type, name)  \
    type _##name;                   \
    type *name = &_##name           \

DECLARE_GLOBAL(__data_segment struct crash_screen, g_crash);   // hmmm

#define BOLD(s)         "\e[1m" s "\e[22m"
#define ITALIC(s)       "\e[3m" s "\e[23m"
#define UNDERLINE(s)    "\e[4m" s "\e[24m"

static void cprint(const char *fmt, ...)
{
    size_t nchars;
    char buf[CRASH_BUFSIZ];

    va_list args;

    va_start(args, fmt);
    nchars = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    console_write(current_console(), buf, nchars);
}

void __fastcall crash(struct iregs *regs)
{
    uint16_t mask;
    bool allow_return = false;

    // get control registers
    uint32_t cr0; read_cr0(cr0);
    uint32_t cr2; read_cr2(cr2);
    uint32_t cr3; read_cr3(cr3);
    uint32_t cr4; cr4 = 0;
    if (cpu_has_cr4()) read_cr4(cr4);

    // get the current and faulting privilege levels
    uint16_t _cs; read_cs(_cs);
    struct segsel *curr_cs = (struct segsel *) &_cs;
    struct segsel *fault_cs = (struct segsel *) &regs->cs;
    int curr_pl = curr_cs->rpl;
    int fault_pl = fault_cs->rpl;

    // did we change privilege levels?
    bool pl_change = false;
    if (curr_pl != fault_pl) {
        pl_change = true;
    }

    // grab the flags
    struct eflags *flags = (struct eflags *) &regs->eflags;

    // was it an IRQ?
    bool irq = (regs->vec_num < 0);
    int irq_num = ~regs->vec_num;

    // an non-maskable interrupt? a page fault?
    bool nmi = (regs->vec_num == EXCEPTION_NMI);
    bool pf = (regs->vec_num == EXCEPTION_PF);

    // locate faulting stack
    uint32_t *stack_ptr;
    if (pl_change) {
        // ESP was pushed to the interrupt stack, thus the ESP value in the
        // iregs structure is valid.
        stack_ptr = (uint32_t *) regs->esp;
     }
     else {
        // SS and ESP are not pushed onto the stack if an interrupt did not
        // change privilege levels, i.e. we are using the same stack. Our common
        // interrupt handler pushed the iregs onto the stack at the time the
        // interrupt occurred, so we must subtract the size of the iregs
        // structure from the stack pointer (excluding ESP and SS) in order to
        // retrieve the top of the faulting stack.
        stack_ptr = (uint32_t *) ((uint32_t) regs + SIZEOF_IREGS_NO_PL_CHANGE);
        regs->esp = (uint32_t) stack_ptr;
    }

    const int NumConsoleCols = (g_vga->cols) ? g_vga->cols : 80;
    const int NumConsoleRows = (g_vga->rows) ? g_vga->rows : 25;

    if (!(irq || nmi)) {
        // fatal crash, does not return
        // Intel Exception
        set_background(CRASH_COLOR);
        cprint("\e[%dH", NumConsoleRows / 4);
        print_banner(" " OS_NAME " ");
        cprint("\n\n");
        cprint("\r\n    A fatal exception " BOLD("%02X") " has occurred at " BOLD("%04X:%08X") ". Press Ctrl+Alt+Del",
            regs->vec_num, regs->cs, regs->eip);
        cprint("\r\n    to restart your computer.");
        cprint("\n");
        cprint("\r\n    *  Exception Name: " BOLD("%s"), exception_names[regs->vec_num]);
        if (pf) {
            cprint("\r\n    *  Description: " BOLD("%s %s Access Violation") " at " BOLD("%08X"),
                regs->err_code & PF_US ? "User" : "Supervisor",
                regs->err_code & PF_WR ? "Write" : "Read", cr2);
            if (!(regs->err_code & PF_P)) {
                cprint("\r\n        -  Page Not Present");
            }
            if (regs->err_code & PF_ID) {
                cprint("\r\n        -  Instruction Fetch Page Fault");
            }
            if (regs->err_code & PF_RSVD) {
                cprint("\r\n        -  Reserved Bit Violation");
            }
        }
        else if (regs->err_code) {
            cprint("\r\n    *  Descriptor: " BOLD("%s(%02X)"),
                (regs->err_code & 0x02) ? "IDT" :
                    (regs->err_code & 0x04) ? "LDT" : "GDT",
                (regs->err_code & 0xFFFF) >> 3,
                (regs->err_code & 0x01) ? " (external)" : "");
        }

        const int StackLines = 8;

        cprint("\e[%dH", NumConsoleRows-StackLines);
        for (int i = 0; i < StackLines; i++) {
            cprint("\r\n");
            if (i==(StackLines-8)+0) cprint(" EAX: %08X EBX: %08X", regs->eax, regs->ebx);
            if (i==(StackLines-8)+1) cprint(" ECX: %08X EDX: %08X", regs->ecx, regs->edx);
            if (i==(StackLines-8)+2) cprint(" ESI: %08X EDI: %08X", regs->esi, regs->edi);
            if (i==(StackLines-8)+3) cprint(" ESP: %08X EBP: %08X", regs->esp, regs->ebp);
            if (i==(StackLines-8)+4) cprint(" EIP: %08X ERR: %08X", regs->eip, regs->err_code);
            if (i==(StackLines-8)+5) cprint(" CR0: %08X CR2: %08X", cr0, cr2);
            if (i==(StackLines-8)+6) cprint(" CR3: %08X CR4: %08X", cr3, cr4);
            if (i==(StackLines-8)+7) {
                cprint(" [");
                if (flags->id)   cprint(" ID");
                // if (flags->vip)  cprint(" VIP");
                // if (flags->vif)  cprint(" VIF");
                // if (flags->ac)   cprint(" AC");
                // if (flags->vm)   cprint(" VM");
                // if (flags->rf)   cprint(" RF");
                // if (flags->nt)   cprint(" NT");
                if (flags->of)   cprint(" OF");
                if (flags->df)   cprint(" DF");
                if (flags->intf) cprint(" IF");
                if (flags->tf)   cprint(" TF");
                if (flags->sf)   cprint(" SF");
                if (flags->zf)   cprint(" ZF");
                if (flags->af)   cprint(" AF");
                if (flags->pf)   cprint(" PF");
                if (flags->cf)   cprint(" CF");
                cprint(" ]");
            }
            cprint("\e[%dG", NumConsoleCols-(8+1)*5);
            cprint("%08X: %08X %08X %08X %08X",
                stack_ptr,
                stack_ptr[0], stack_ptr[1],
                stack_ptr[2], stack_ptr[3]);
            stack_ptr += 4;
        }
        goto done;
    }

    // backup frame buffer and console state
    char old_fb[FB_SIZE];
    struct console_save_state saved_console;
    memcpy(old_fb, get_console_fb(g_vga->active_console), FB_SIZE);
    console_save(current_console(), &saved_console);

    if (irq) {
        allow_return = true;
        set_background(IRQ_COLOR);
        cprint("\e[%dH", NumConsoleRows / 3);
        print_banner(" Unhandled Interrupt ");
        cprint("\n\n\e[1m");
        cprint("\r\n    An unhandled interrupt was raised by device " UNDERLINE("IRQ %d") ". This could mean the", irq_num);
        cprint("\r\n    device is misconfigured or malfunctioning. If this persists, press");
        cprint("\r\n    Ctrl+Alt+Del to restart your computer.");
        cprint("\n\n\n");
        center_text("Press any key to continue \e6");
    }
    else if (nmi) {
        allow_return = true;
        set_background(NMI_COLOR);
        cprint("\e[%dH", NumConsoleRows / 3);
        print_banner(" Non-Maskable Interrupt ");
        cprint("\n\n\e[1m");
        cprint("\r\n    A non-maskable interrupt was raised. If this persists, press Ctrl+Alt+Del");
        cprint("\r\n    to restart your computer.");
        cprint("\n\n\n");
        center_text("Press any key to continue \e6");
    }
    else {

    }
    cprint("\e[0m");

done:
    mask = pic_getmask();
    pic_setmask(0xFFFF);
    pic_unmask(IRQ_KEYBOARD);

    if (!allow_return) {
        for (;;);   // spin forever
    }
    console_flush(current_console());

    __sti();
    console_getchar(current_console());
    __cli();

    // restore console state
    console_restore(current_console(), &saved_console);
    memcpy(get_console_fb(g_vga->active_console), old_fb, FB_SIZE);
    pic_setmask(mask);
    (void) mask;
}

static void center_text(const char *str, ...)
{
    va_list args;
    char buf[g_vga->cols];
    int len;
    int col;
    char c;
    int esc;
    char *p;

    va_start(args, str);
    vsnprintf(buf, sizeof(buf), str, args);
    va_end(args);

    esc = 0;
    len = 0;
    p = buf;

    while ((c = *p++) != '\0') {
        if (c == '\e') {
            esc = 1;
            continue;
        }
        if (esc == 1 && c == '[') {
            esc = 2;
            continue;
        }

        if (esc == 1) {
            esc = 0;        // terminate non-CSI escape sequence (ESC + 1 char)
        }
        if (esc == 2) {
            if (c == ';' || isdigit(c)) {
                continue;   // skip CSI params (<n>;<m>;...)
            }
            esc = 0;        // any other char, terminate CSI escape sequence
            continue;
        }
        if (iscntrl(c)) {
            continue;
        }

        assert(esc == 0);
        len++;
    }

    col = (g_vga->cols - len) / 2;
    if (col < 0) {
        col = 0;
    }
    cprint("\e[%dG%s", col, buf);
}


static void print_banner(const char *banner)
{
    cprint("\e[47;3%dm", BANNER_COLOR);
    center_text(banner);
    cprint("\e[37;4%dm", g_crash->bg);
}

static void set_background(int bg)
{
    g_crash->bg = bg;
    cprint("\e5\e[0H\e[4%dm\e[2J", bg);
}

static const char *exception_names[NR_EXCEPTIONS] =
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
static_assert(countof(exception_names) == NR_EXCEPTIONS, "Bad exception_names length");
