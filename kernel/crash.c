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

// colors
#define NMI_COLOR           CSI_RED
#define IRQ_COLOR           CSI_RED
#define CRASH_COLOR         CSI_BLUE
#define BANNER_COLOR        CSI_BLUE

// text positioning
#define NMI_HEADER_SCALE    3
#define IRQ_HEADER_SCALE    3
#define CRASH_HEADER_SCALE  4   // header text positioning scale factor, 0=center
#define CRASH_DUMP_OFFSET   1   // register/stack dump offset from bottom

// stack dump dimensions
#define STACK_ROWS          8
#define STACK_COLUMNS       2

// optional visual information
#define SHOW_SEGMENT_REGS   1   // should we dump segment registers?
#define SHOW_ESP_ARROW      1   // show arrow pointing to top of stack?

// fallback text-mode screen dimensions,
// in case a crash occurs very early before we are able to collect VGA info
#define DEFAULT_VGA_ROWS    25
#define DEFAULT_VGA_COLUMNS 80

// convenient ANSI escape sequence wrappers
#define BOLD(s)             "\e[1m" s "\e[22m"
#define ITALIC(s)           "\e[3m" s "\e[23m"
#define UNDERLINE(s)        "\e[4m" s "\e[24m"

static const char *exception_names[NR_EXCEPTIONS];

struct crash_screen {
    bool reentrant;
    int bg_color;
};

__data_segment struct crash_screen _crash;
__data_segment struct crash_screen *g_crash = &_crash;

extern struct vga *g_vga;

#if DEBUG
extern int g_crash_test_double_fault;
#endif

static void print_regs_and_stack(struct iregs *regs);
static void print_segsel(struct segsel *segsel);
static void print_flags(struct eflags *flags);


static void cprint(const char *fmt, ...);
static void set_background(int bg);
static void print_banner(const char *banner);
static void center_text(const char *str, ...);


void __fastcall crash(struct iregs *regs)
{
    uint16_t mask;
    bool allow_return = false;

    //
    // basic info collection
    //

    // get necessary control registers
    uint32_t cr2; read_cr2(cr2);

    // get current segment registers
    struct segsel *cs, *ss;
    cs = (struct segsel *) &regs->cs;
    ss = (struct segsel *) &regs->ss;
    uint32_t _cs; read_cs(_cs);
    uint32_t _ss; read_ss(_ss);

    // get the current and faulting privilege levels
    struct segsel *curr_cs = (struct segsel *) &_cs;
    int curr_pl = curr_cs->rpl;
    int fault_pl = cs->rpl;

    // did we change privilege levels?
    bool pl_change = false;
    if (curr_pl != fault_pl) {
        pl_change = true;
    }

    // locate the faulting stack and associated stack segment selector
    if (!pl_change) {
        ss = (struct segsel *) &_ss;
    }
    uint32_t * const esp = (pl_change)
        ? (uint32_t * const) regs->esp
        : (uint32_t * const) (((char *) regs) + SIZEOF_IREGS_NO_PL_CHANGE);

    // fixup the regs pointer,
    //   make a copy and assign correct SS and ESP,
    //   then reassign regs pointer to point to local copy
    struct iregs _regs_copy = *regs;
    _regs_copy.ss = ss->_value;
    _regs_copy.esp = (uint32_t) esp;
    regs = &_regs_copy;

    //
    // double fault handler, to catch exceptions that somehow occur while trying
    // to show the crash screen. this is a "software" double fault, as opposed
    // to a "true" double fault occurs when, for example, the CPU encounters a
    // page fault while handling a previous page fault.
    //

    // reentrancy check
    if (g_crash->reentrant) {
        // If we end up here, we hit an exception while trying to show the
        // crash screen... VERY BAD! Show bare minimum information here, then
        // spin. Less code the better; that's unlikely to fail.
        cprint("\r\n\r\n\e[0m\e[31m");
        cprint("fatal: double fault caused by %s at %04X:%08X\e[0m",
            exception_names[regs->vec_num], regs->cs, regs->eip);
        cprint("\r\n\r\n");
        center_text("==> REGISTER AND STACK DUMP <==\r\n");
        print_regs_and_stack(regs);
        cprint("\r\n\r\n");
        center_text("Press Ctrl+Alt+Del to restart your computer.");
        cprint("\r\n\e5");
        goto done;
    }
    g_crash->reentrant = true;  // catch a double-fault

#if DEBUG
    // test a double fault
    if (g_crash_test_double_fault) {
        __asm__ volatile (".short 0x0A0F");
    }
#endif

    //
    // handle fatal crash
    //

    // was it an IRQ?
    bool irq = (regs->vec_num < 0);
    int irq_num = ~regs->vec_num;

    // an non-maskable interrupt? a page fault? a double fault?
    bool nmi = (regs->vec_num == EXCEPTION_NMI);
    bool pf = (regs->vec_num == EXCEPTION_PF);
    bool df = (regs->vec_num == EXCEPTION_DF);

    // NOTE: PCem doesn't push an error code of 0 onto the stack when a true
    // double fault occurs, which violates the Intel spec. Source code backs
    // this up. As a result, a software double fault will occur here when
    // attempting to print the stack contents because the iregs struture is
    // misaligned due to the missing error code.
    // TODO: file a bug on PCem for this!!

    const int NumConsoleRows = (g_vga->rows) ? g_vga->rows : DEFAULT_VGA_ROWS;
    const int OffsetFromBottom = (NumConsoleRows-CRASH_DUMP_OFFSET)-STACK_ROWS;

    if (!(irq || nmi)) {
        set_background(CRASH_COLOR);
        cprint("\e[%dH", NumConsoleRows / CRASH_HEADER_SCALE);
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
        else if (regs->err_code && !df) {
            cprint("\r\n    *  Descriptor: " BOLD("%s(%02X)"),
                (regs->err_code & 0x02) ? "IDT" :
                    (regs->err_code & 0x04) ? "LDT" : "GDT",
                (regs->err_code & 0xFFFF) >> 3,
                (regs->err_code & 0x01) ? " (external)" : "");
        }

        cprint("\e[%dH", OffsetFromBottom);
        print_regs_and_stack(regs);
        goto done;
    }

    //
    // handle warnings / alerts, user can press a key to return to running task
    //

    // backup frame buffer and console state
    char old_fb[FB_SIZE];
    struct console_save_state saved_console;
    memcpy(old_fb, get_console_fb(g_vga->active_console), FB_SIZE);
    console_save(current_console(), &saved_console);

    assert(irq || nmi);

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
    cprint("\e[0m");

done:
    mask = pic_getmask();
    pic_setmask(0xFFFF);
    pic_unmask(IRQ_KEYBOARD);

    if (!allow_return) {
        // spin forever
        __sti(); for (;;);
    }

    // wait for keypress
    console_flush(current_console());
    __sti();
    console_getchar(current_console());
    __cli();

    // restore console state
    console_restore(current_console(), &saved_console);
    memcpy(get_console_fb(g_vga->active_console), old_fb, FB_SIZE);
    pic_setmask(mask);

    g_crash->reentrant = false;
}

static void print_regs_and_stack(struct iregs *regs)
{
    const int NumConsoleCols = (g_vga->cols) ? g_vga->cols : DEFAULT_VGA_COLUMNS;
    const int StackStartCol = NumConsoleCols-((8+1)*STACK_COLUMNS);

    uint32_t *stack_ptr = (uint32_t *) regs->esp;
    struct eflags *flags = (struct eflags *) &regs->eflags;

    uint32_t cr0; read_cr0(cr0);
    uint32_t cr2; read_cr2(cr2);
    uint32_t cr3; read_cr3(cr3);
    uint32_t cr4; cr4 = 0;
    if (cpu_has_cr4()) read_cr4(cr4);

#if SHOW_SEGMENT_REGS
    struct segsel *cs, *ds, *es, *fs, *gs, *ss;
    cs = (struct segsel *) &regs->cs;
    ds = (struct segsel *) &regs->ds;
    es = (struct segsel *) &regs->es;
    fs = (struct segsel *) &regs->fs;
    gs = (struct segsel *) &regs->gs;
    ss = (struct segsel *) &regs->ss;
#endif

    for (int i = 0; i < STACK_ROWS; i++) {
        cprint("\r\n ");
        if (i==(STACK_ROWS-8)+0) { print_flags(flags); }
        if (i==(STACK_ROWS-8)+1) { cprint("EAX=%08X EBX=%08X", regs->eax, regs->ebx); }
        if (i==(STACK_ROWS-8)+2) { cprint("ECX=%08X EDX=%08X", regs->ecx, regs->edx); }
        if (i==(STACK_ROWS-8)+3) { cprint("ESI=%08X EDI=%08X", regs->esi, regs->edi); }
        if (i==(STACK_ROWS-8)+4) { cprint("ESP=%08X EBP=%08X", regs->esp, regs->ebp); }
        if (i==(STACK_ROWS-8)+5) { cprint("EIP=%08X ERR=%08X", regs->eip, regs->err_code); }
        if (i==(STACK_ROWS-8)+6) { cprint("CR0=%08X CR2=%08X", cr0, cr2); }
        if (i==(STACK_ROWS-8)+7) { cprint("CR3=%08X CR4=%08X", cr3, cr4); }

#if SHOW_SEGMENT_REGS
        if (i==(STACK_ROWS-6)+0) { cprint("  SS="); print_segsel(ss); }
        if (i==(STACK_ROWS-6)+1) { cprint("  CS="); print_segsel(cs); }
        if (i==(STACK_ROWS-6)+2) { cprint("  DS="); print_segsel(ds); }
        if (i==(STACK_ROWS-6)+3) { cprint("  ES="); print_segsel(es); }
        if (i==(STACK_ROWS-6)+4) { cprint("  FS="); print_segsel(fs); }
        if (i==(STACK_ROWS-6)+5) { cprint("  GS="); print_segsel(gs); }
#endif

        cprint("\e[%dG", StackStartCol);
#if SHOW_ESP_ARROW
        if (i == 0) { cprint("\e[6DESP-->"); }
#endif
        for (int k = 0; k < STACK_COLUMNS; k++, stack_ptr++) {
            cprint(" %08X", stack_ptr[i]);
        }
    }
}

static void print_segsel(struct segsel *segsel)
{
    volatile struct table_desc _gdt_desc = { }; __sgdt(_gdt_desc);
    struct x86_desc *gdt = (struct x86_desc *) _gdt_desc.base;
    struct x86_desc *desc = x86_get_desc(gdt, segsel->_value);

    cprint("%02X(%02X|%d|%d):",
        segsel->_value, segsel->index,
        segsel->ti, segsel->rpl);

    if (x86_desc_valid(_gdt_desc, desc)) {
        cprint("%08X,%05X %d %d",
            x86_seg_base(desc), x86_seg_limit(desc),
            desc->seg.g, desc->seg.db);
    }
    else {
        cprint("(invalid)");
    }
}

static void print_flags(struct eflags *flags)
{
    cprint("[");
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

static void cprint(const char *fmt, ...)
{
    const int BufferSize = 512;

    size_t nchars;
    char buf[BufferSize];

    va_list args;

    va_start(args, fmt);
    nchars = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    console_write(current_console(), buf, nchars);
}

static void set_background(int bg)
{
    g_crash->bg_color = bg;
    cprint("\e5\e[0H\e[4%dm\e[2J", bg);
}

static void print_banner(const char *banner)
{
    cprint("\e[47;3%dm", BANNER_COLOR);
    center_text(banner);
    cprint("\e[37;4%dm", g_crash->bg_color);
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
