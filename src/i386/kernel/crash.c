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
 *         File: i386/kernel/crash.c
 *      Created: May 31, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ctype.h>
#include <stdarg.h>
#include <i386/cpu.h>
#include <i386/interrupt.h>
#include <i386/x86.h>
#include <kernel/kernel.h>
#include <kernel/ohwes.h>
#include <kernel/irq.h>
#include <kernel/terminal.h>
#include <kernel/vga.h>

#define CRASH_BUFSIZ        1024
#define CRASH_COLOR         ANSI_BLUE
#define CRASH_MARGIN        5
#define CRASH_SCALE         2

#define MSG_TAIL            "The system cannot be recovered and must be restarted."
#define MSG_PROMPT          "Press CTRL+ALT+DEL to restart your computer "

// convenient ANSI escape sequence wrappers
#define BOLD(s)             "\e[1m"  s "\e[22m"
#define ITALIC(s)           "\e[3m"  s "\e[23m"
#define UNDERLINE(s)        "\e[4m"  s "\e[24m"
#define RED(s)              "\e[31m" s "\e[39m"

// optional visual information
#define DUMP_SEGMENT_REGS   0
#define DUMP_MM_REGS        0
#define DUMP_STACK          1

// stack dump dimensions
#define STACK_DUMP_ROWS     8
#define STACK_DUMP_COLS     4

#if DEBUG
int g_test_crashkey = 0;
int g_test_soft_double_fault = 0;
#endif

extern struct console *g_consoles;

static const char *exception_names[NR_EXCEPTIONS];

static void cprint(const char *fmt, ...);
static void fbprint(const char *fmt, ...);
static void fbwrite(const char *buf, size_t count);

static void center_text(int maxwidth, const char *fmt, ...);
static void wrap_text(int margin, const char *fmt, ...);

typedef void (*dumpfn)(const char *fmt, ...);

// all the "dump" functions print a leading newline
static void dump_cpu(struct cpu_state *cpu, dumpfn to);
static void dump_cntlregs(struct cpu_state *cpu, dumpfn to);
static void dump_gpregs(struct cpu_state *cpu, dumpfn to);
static void dump_segregs(struct cpu_state *cpu, dumpfn to);
static void dump_mmregs(struct cpu_state *cpu, dumpfn to);
static void dump_stack(struct cpu_state *cpu, dumpfn to);
static void dump_segsel(struct segsel *segsel, dumpfn to);

//
// Capture the extraneous CPU state and combine with interrupt regs into a
// cpu state object.
//
void capture_cpu_state(struct cpu_state *state, struct iregs *iregs)
{
    state->iregs.ebx = iregs->ebx;
    state->iregs.ecx = iregs->ecx;
    state->iregs.edx = iregs->edx;
    state->iregs.esi = iregs->esi;
    state->iregs.edi = iregs->edi;
    state->iregs.ebp = iregs->ebp;
    state->iregs.eax = iregs->eax;
    state->iregs.ds  = iregs->ds;
    state->iregs.es  = iregs->es;
    state->iregs.fs  = iregs->fs;
    state->iregs.gs  = iregs->gs;
    state->iregs.vec_num = iregs->vec_num;
    state->iregs.err_code = iregs->err_code;
    state->iregs.eip = iregs->eip;
    state->iregs.cs = iregs->cs;
    state->iregs.eflags = iregs->eflags;
    state->iregs.esp = get_esp(iregs);
    state->iregs.ss = get_ss(iregs);
    store_cr0(state->cr0);
    store_cr2(state->cr2);
    store_cr3(state->cr3);
    state->cr4 = 0; if (cpu_has_cr4()) store_cr4(state->cr4);
    __sgdt(state->gdtr);
    __sidt(state->idtr);
    __sldt(state->ldtr);
    __str(state->tr);
}

static void wait_for_keypress(void)
{
    struct terminal *term;
    struct termios orig_termios;
    uint16_t mask;

    term = get_terminal(0);
    if (term->tty) {
        orig_termios = term->tty->termios;
        term->tty->termios.c_lflag &= ~(ECHO);
    }

    mask = irq_getmask();
    irq_setmask(IRQ_MASKALL);
    if (_IRQ_ENABLED(mask, IRQ_TIMER)) {
        irq_unmask(IRQ_TIMER);
    }
    if (_IRQ_ENABLED(mask, IRQ_TIMER)) {
        irq_unmask(IRQ_KEYBOARD);
    }

    irq_enable();

    kb_getc();   // blocks until a char is sent

    irq_disable();
    irq_setmask(mask);

    if (term->tty) {
        term->tty->termios = orig_termios;
    }
}

static void show_crash_screen(
    int color, int margin,
    const char *banner,
    const char *primary_text,
    const char *secondary_text)
{
    const int MaxWidth = vga_get_cols();
    const int MaxHeight = vga_get_rows();

    fbprint("\e[22;4%d;37m\e[2J", color & 7);
    fbprint("\e[%dH", MaxHeight / 3);
    center_text(MaxWidth, "\e[7m %s \e[27m", banner);
    fbprint("\n\n\e[1m");
    wrap_text(margin, primary_text);
    fbprint("\n\n");
    center_text(MaxWidth, secondary_text);
}

//
// Uh oh! An exception occurred in the exception handler.
// Do the bare minimum here to show diagnostic information to the user.
//
// This is to be called ONLY by handle_exception() if we were previously
// handling an exception!
//
__noreturn void handle_soft_double_fault(
    struct cpu_state *cpu, struct cpu_state *orig_cpu)
{
    char msgbuf[CRASH_BUFSIZ];

    cprint("\n\n\e[1m" RED("*** FATAL: exception (1) occurred while handling previous exception (2)"));
    cprint("\n\n(1) %s at %08X", exception_names[cpu->iregs.vec_num], cpu->iregs.eip);
    dump_cpu(cpu, cprint);
    cprint("\n\n(2) %s at %08X", exception_names[orig_cpu->iregs.vec_num], orig_cpu->iregs.eip);
    dump_cpu(orig_cpu, cprint);

    snprintf(msgbuf, sizeof(msgbuf),
        "An exception %02X (%s) has occurred at %08X while handling a previous "
        "exception %02X (%s) that occurred at %08X. "
        MSG_TAIL,
        cpu->iregs.vec_num, exception_names[cpu->iregs.vec_num], cpu->iregs.eip,
        orig_cpu->iregs.vec_num, exception_names[orig_cpu->iregs.vec_num], orig_cpu->iregs.eip);

    show_crash_screen(ANSI_RED, 5, "Double Fault", msgbuf, MSG_PROMPT);

    while (true) {
        wait_for_keypress();
    }
}

//
// Generic x86 exception handler.
//
__fastcall void handle_exception(struct iregs *iregs)
{
    // static vars for soft double-fault detection
    static bool crashing = false;
    static struct cpu_state orig_cpu;

    char msgbuf[CRASH_BUFSIZ];
    char errbuf[CRASH_BUFSIZ];
    struct cpu_state cpu;

    // get the remaining regs
    capture_cpu_state(&cpu, iregs);

    // if we're already crashing... well... that's not good... handle it here!
    if (crashing) {
        handle_soft_double_fault(&cpu, &orig_cpu);
    }
    crashing = true;
    orig_cpu = cpu;

#if DEBUG
    // test a software double fault
    if (g_test_soft_double_fault) {
        __asm__ volatile (".short 0x0A0F");
    }
#endif

    cprint("\n\n\e[1m" RED("*** FATAL: exception %02X occurred at %08X") "\n",
        iregs->vec_num, exception_names[iregs->vec_num]);
    dump_cpu(&cpu, cprint);

    // collect error info
    if (iregs->vec_num == PAGE_FAULT) {
        int us = iregs->err_code & PF_US;
        int wr = iregs->err_code & PF_WR;
        int p = iregs->err_code & PF_P;
        int rsvd = iregs->err_code & PF_RSVD;
        snprintf(errbuf, CRASH_BUFSIZ, " A %s mode %s %08X caused a %s.",
            (us) ? "user" : "kernel", (wr) ? "write to" : "read from", cpu.cr2,
            (p)
                ? (rsvd) ? "reserved bit violation" : "access violation"
                : "non-present page access violation");
    }
    else if (iregs->err_code) {
        snprintf(errbuf, sizeof(errbuf), " The issue occurred in %s(%02X)%s.",
            (iregs->err_code & ERR_IDT) ? "IDT" :
                (iregs->err_code & ERR_TI) ? "LDT" : "GDT",
            (iregs->err_code & ERR_INDEX) >> 3,
            (iregs->err_code & ERR_EXT) ? " and originated via an interrupt" : "");
    }
    else {
        snprintf(errbuf, sizeof(errbuf), "");
    }

    snprintf(msgbuf, sizeof(msgbuf),
        "A fatal exception %02X (%s) has occurred at %08X.%s "
        MSG_TAIL,
        iregs->vec_num, exception_names[iregs->vec_num],
        iregs->eip, errbuf);

    show_crash_screen(CRASH_COLOR, CRASH_MARGIN, OS_NAME, msgbuf, MSG_PROMPT);
    // dump_cpu(&cpu, fbprint);

    while (true) {
        wait_for_keypress();
    }
    crashing = false;   // in case we ever return...
}

//
// x86 Double Fault exception handler. An exception occurred within the CPU
// while handling a different exception. Called by hardware; not to be called
// directly.
//
// The IDT is set up to perform a task switch if a Double Fault exception
// occurs, via a task gate. This is to ensure we end up with a known good stack
// so we can print diagnostic information to the user. Grab the program context
// from the faulting program's TSS and feed it to handle_exception().
//
__noreturn void handle_double_fault(void)
{
    struct tss *tss = get_tss();
    struct tss *fault_tss = get_tss_from_gdt(tss->prev_task);

    struct iregs regs = { };
    regs.ebx = fault_tss->ebx;
    regs.ecx = fault_tss->ecx;
    regs.edx = fault_tss->edx;
    regs.esi = fault_tss->esi;
    regs.edi = fault_tss->edi;
    regs.ebp = fault_tss->ebp;
    regs.eax = fault_tss->eax;
    regs.ds  = fault_tss->ds;
    regs.es  = fault_tss->es;
    regs.fs  = fault_tss->fs;
    regs.gs  = fault_tss->gs;
    regs.vec_num = DOUBLE_FAULT;
    regs.err_code = 0;
    regs.eip = fault_tss->eip;
    regs.cs  = fault_tss->cs;
    regs.eflags = fault_tss->eflags;
    regs.esp = fault_tss->esp;
    regs.ss  = fault_tss->ss;

    handle_exception(&regs);
    for (;;);
}

static void dump_cpu(struct cpu_state *cpu, dumpfn dump)
{
#if DUMP_STACK
    // cprint("\n");
    dump_stack(cpu, dump);
#endif

    if (cpu->iregs.err_code) {
        dump("\nERR=%08X", cpu->iregs.err_code);
    }

    dump_cntlregs(cpu, dump);
    dump_gpregs(cpu, dump);

#if DUMP_SEGMENT_REGS
    // cprint("\n");
    dump_segregs(cpu, dump);
#endif

#if DUMP_MM_REGS
    dump_mmregs(cpu, dump);
#endif

}

static void dump_cntlregs(struct cpu_state *cpu, dumpfn dump)
{
    dump("\nCR0=%08X CR2=%08X CR3=%08X CR4=%08X",
        cpu->cr0, cpu->cr2, cpu->cr3, cpu->cr4);
}

static void dump_gpregs(struct cpu_state *cpu, dumpfn dump)
{
    struct iregs *regs = &cpu->iregs;
    struct eflags *flags = (struct eflags *) &regs->eflags;

    dump("\nEAX=%08X EBX=%08X ECX=%08X EDX=%08X",
        regs->eax, regs->ebx, regs->ecx, regs->edx);
    dump("\nESI=%08X EDI=%08X ESP=%08X EBP=%08X",
        regs->esi, regs->edi, regs->esp, regs->ebp);
    dump("\nEIP=%08X ", regs->eip);

    dump("EFL=%08x [", flags->_value);
    if (flags->id)   dump(" ID");
    if (flags->vip)  dump(" VIP");
    if (flags->vif)  dump(" VIF");
    if (flags->ac)   dump(" AC");
    if (flags->vm)   dump(" VM");
    if (flags->rf)   dump(" RF");
    if (flags->nt)   dump(" NT");
    dump(" IOPL=%d", flags->iopl);
    if (flags->of)   dump(" OF");
    if (flags->df)   dump(" DF");
    if (flags->intf) dump(" IF");
    if (flags->tf)   dump(" TF");
    if (flags->sf)   dump(" SF");
    if (flags->zf)   dump(" ZF");
    if (flags->af)   dump(" AF");
    if (flags->pf)   dump(" PF");
    if (flags->cf)   dump(" CF");
    dump(" ]");
}

static void dump_segregs(struct cpu_state *cpu, dumpfn dump)
{
    dump("\nSS="); dump_segsel((struct segsel *) &cpu->iregs.ss, dump);
    dump("\nCS="); dump_segsel((struct segsel *) &cpu->iregs.cs, dump);
    dump("\nDS="); dump_segsel((struct segsel *) &cpu->iregs.ds, dump);
    dump("\nES="); dump_segsel((struct segsel *) &cpu->iregs.es, dump);
    dump("\nFS="); dump_segsel((struct segsel *) &cpu->iregs.fs, dump);
    dump("\nGS="); dump_segsel((struct segsel *) &cpu->iregs.gs, dump);
}

static void dump_mmregs(struct cpu_state *cpu, dumpfn dump)
{
    struct table_desc *gdt_desc = (struct table_desc *) &cpu->gdtr;
    struct table_desc *idt_desc = (struct table_desc *) &cpu->idtr;

    dump("\nTR="); dump_segsel((struct segsel *) &cpu->tr, dump);
    dump("\nLDTR="); dump_segsel((struct segsel *) &cpu->ldtr, dump);
    dump("\nGDTR=%08X,%05X IDTR=%08X,%05X",
        gdt_desc->base, gdt_desc->limit, idt_desc->base, idt_desc->limit);
}

static void dump_stack(struct cpu_state *cpu, dumpfn dump)
{
    const uint32_t *esp = (const uint32_t *) cpu->iregs.esp;
    const uint32_t *ebp = (const uint32_t *) cpu->iregs.ebp;

    for (int i = 0; i < STACK_DUMP_ROWS
            && ((uint32_t) esp % PAGE_SIZE) != 0
            && esp < ebp;
        i++)
    {
        dump("\n%08X:", esp);
        for (int k = 0; k < STACK_DUMP_COLS
                && ((uint32_t) esp % PAGE_SIZE) != 0
                && esp < ebp;
            k++, esp++)
        {
            dump(" %08X", *((uint32_t *) esp));
        }
    }
}

static void dump_segsel(struct segsel *segsel, dumpfn dump)
{
    volatile struct table_desc _gdt_desc = { }; __sgdt(_gdt_desc);
    struct x86_desc *gdt = (struct x86_desc *) _gdt_desc.base;
    struct x86_desc *desc = x86_get_desc(gdt, segsel->_value);

    dump("%02X(%02X|%d|%d):",
        segsel->_value, segsel->index,
        segsel->ti, segsel->rpl);

    if (x86_desc_valid(_gdt_desc, desc)) {
        dump("%08X,%05X %d %d",
            x86_seg_base(desc), x86_seg_limit(desc),
            desc->seg.g, desc->seg.db);
    }
    else {
        dump("(invalid)");
    }
}

// like kprint but with a smaller stack footprint and will write directly to
// frame buffer if no console is registered
void cprint(const char *fmt, ...)
{
    char buf[CRASH_BUFSIZ] = { };
    va_list args;
    size_t count;

    va_start(args, fmt);
    count = vsnprintf(buf, CRASH_BUFSIZ, fmt, args);
    va_end(args);

    if (has_console()) {
        console_write(buf, count);
    }
    else {
        fbwrite(buf, count);
    }
}

// print directly to active the terminal's VGA frame buffer
static void fbprint(const char *fmt, ...)
{
    va_list args;
    size_t count;
    char buf[CRASH_BUFSIZ] = { };

    va_start(args, fmt);
    count = vsnprintf(buf, CRASH_BUFSIZ, fmt, args);
    va_end(args);

    fbwrite(buf, count);
}

static void fbwrite(const char *buf, size_t count)
{
    struct terminal *term;
    const char *p;

    term = get_terminal(0);
    if (!term->initialized) {
        terminal_defaults(term);
        term->number = 1;
        term->framebuf = get_vga_fb();
        term->initialized = true;
    }

    p = buf;
    while (p < buf + count) {
        if (*p == '\n') {
            terminal_putchar(term, '\r');
        }
        p += terminal_putchar(term, *p);
    }
}

static void center_text(int maxwidth, const char *fmt, ...)
{
    int len;
    int col;
    char c;
    int esc;
    const char *p;
    char buf[CRASH_BUFSIZ];
    va_list args;

    // TODO: handle multiple lines
    //       handle wrap

    if (maxwidth < 0) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(buf, CRASH_BUFSIZ, fmt, args);
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

    col = (maxwidth - len) / 2;
    if (col < 0) {
        col = 0;
    }

    fbprint("\e[%dG%s", col, buf);
}

static void wrap_text(int margin, const char *fmt, ...)
{
    const int MaxWidth = vga_get_cols();

    // technically "wrap and left justify within margin"

    va_list args;
    char buf[CRASH_BUFSIZ];
    const char *word;
    const char *p;
    int linelen;
    int wordlen;
    int esclen;
    bool print_margin;
    int esc;

    if (margin < 0) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(buf, CRASH_BUFSIZ, fmt, args);
    va_end(args);

    esc = 0;
    esclen = 0;
    wordlen = 0;
    linelen = 2 * margin;
    print_margin = true;

    p = buf;
    word = p;

    while ((p - buf) < CRASH_BUFSIZ && *p != '\0') {
        // find end of word by looking for space
        word = p;
        wordlen = 0;
        esclen = 0;
        for (; (p - buf) < CRASH_BUFSIZ && *p != '\0'; p++) {
            if (linelen > MaxWidth) {
                fbprint("\r\n");
                linelen = (2 * margin) + wordlen;
                print_margin = true;
            }

            // TODO: handle tabs?

            if (*p == '\n') {
                fbprint("%.*s", wordlen + esclen, word);
                linelen = (2 * margin);
                print_margin = true;
                break;
            }
            if (*p == '\e') {
                esc = 1;
                esclen++;
                continue;       // begin esc
            }

            if (esc) {
                esclen++;
            }
            if (esc == 1) {
                if (*p == '[') {
                    esc = 2;    // CSI esc
                    continue;
                }
                esc = 0;        // end esc
                continue;
            }
            if (esc == 2) {
                if (isdigit(*p) || *p == ';') {
                    continue;
                }
                esc = 0;
                continue;
            }

            if (print_margin) {
                for (int i = 0; i < margin; i++) {
                    fbprint(" ");
                }
                print_margin = false;
            }

            if (isspace(*p)) {
                fbprint("%.*s", wordlen + esclen, word);
                break;
            }

            wordlen++;
            linelen++;
        }

        // eat up trailing spaces
        for (; isspace(*p) && *p != '\0'; p++) {
            if (linelen > MaxWidth) {
                fbprint("\r\n");
                linelen = (2 * margin) + wordlen;
                print_margin = true;
                break;
            }

            if (linelen > 0) {
                linelen++;
                fbprint("%c", *p);
            }
        }
    }
    fbprint("%.*s", wordlen + esclen, word);
}

#ifdef DEBUG
void crash_key_irq(int irq, struct iregs *regs)   // TODO: call this vis sysreq...
{
    int crash_type;
    if (g_test_crashkey <= 0) {
        return;
    }

    crash_type = g_test_crashkey;
    g_test_crashkey = -1;
    (void) irq;

    // pick your poison
    switch (crash_type) {
        case 1:     // F1 - divide by zero
            __asm__ volatile ("idiv %0" :: "a"(0), "b"(0));
            break;
        case 2:     // F2 - simulate nmi (TODO: real NMI possible?)
            __asm__ volatile ("int $2");
            break;
        case 3:     // F3 - debug break
            __asm__ volatile ("int $3");
            break;
        case 4:     // F4 - panic()
            panic("you fucked up!!");
            break;
        case 5:     // F5 - assert()
            assert(true == false);
            break;
        case 6:     // F6 - unexpected device interrupt vector
            __asm__ volatile ("int $0x2D");
            break;
        // case 7:     // F7 - kernel stack page fault
        //     __asm__ volatile ("movl $0, %esp; popl %eax");
        //     break;

        case 7:     // F7 - spurious interrupt
            __asm__ volatile ("int $0x27");
            break;

        case 8: {   // F8 - nullptr read
            volatile uint32_t *badptr = NULL;
            const int bad = *badptr;
            (void) bad;
            break;
        }
        case 9: {   // F9 - bad ptr write
            volatile uint32_t *badptr = (uint32_t *) 0xCA55E77E;
            *badptr = 0xBADC0DE;
            break;
        }
        case 10: {  // F10 - software double fault
            kprint("\nsoft double fault...");
            g_test_soft_double_fault = true;
            __asm__ volatile ("idiv %0" :: "a"(0), "b"(0));
            break;
        }
        case 11: {  // F11 - true double fault
            kprint("\ndouble fault...");
            volatile struct x86_desc *idt;
            idt = get_idt();
            idt[BREAKPOINT_EXCEPTION].trap.p = 0;
            idt[SEGMENT_NOT_PRESENT].trap.p = 0;
            __asm__ volatile("int3");
            break;
        }
        case 12: {  // F12 - triple fault
            kprint("\ntriple fault...");
            struct table_desc idt_desc = { .limit = 0, .base = 0 };
            __lidt(idt_desc);   // yoink away the IDT :D
            break;
        }
    }
}
#endif

static const char *exception_names[NR_EXCEPTIONS] =
{
    /*0x00*/ "DIVIDE_ERROR",
    /*0x01*/ "DEBUG_EXCEPTION",
    /*0x02*/ "NON_MASKABLE_INTERRUPT",
    /*0x03*/ "BREAKPOINT_EXCEPTION",
    /*0x04*/ "OVERFLOW_EXCEPTION",
    /*0x05*/ "BOUND_RANGE_EXCEEDED",
    /*0x06*/ "INVALID_OPCODE",
    /*0x07*/ "DEVICE_NOT_AVAILABLE",
    /*0x08*/ "DOUBLE_FAULT",
    /*0x09*/ "SEGMENT_OVERRUN",
    /*0x0A*/ "INVALID_TSS",
    /*0x0B*/ "SEGMENT_NOT_PRESENT",
    /*0x0C*/ "STACK_FAULT",
    /*0x0D*/ "GENERAL_PROTECTION_FAULT",
    /*0x0E*/ "PAGE_FAULT",
    /*0x0F*/ "INVALID_EXCEPTION_0F",
    /*0x10*/ "MATH_FAULT",
    /*0x11*/ "ALIGNMENT_CHECK",
    /*0x12*/ "MACHINE_CHECK",
    /*0x13*/ "SIMD_FAULT",
    /*0x14*/ "INVALID_EXCEPTION_14",
    /*0x15*/ "INVALID_EXCEPTION_15",
    /*0x16*/ "INVALID_EXCEPTION_16",
    /*0x17*/ "INVALID_EXCEPTION_17",
    /*0x18*/ "INVALID_EXCEPTION_18",
    /*0x19*/ "INVALID_EXCEPTION_19",
    /*0x1A*/ "INVALID_EXCEPTION_1A",
    /*0x1B*/ "INVALID_EXCEPTION_1B",
    /*0x1C*/ "INVALID_EXCEPTION_1C",
    /*0x1D*/ "INVALID_EXCEPTION_1D",
    /*0x1E*/ "INVALID_EXCEPTION_1E",
    /*0x1F*/ "INVALID_EXCEPTION_1F",
};
static_assert(
    countof(exception_names) == NR_EXCEPTIONS, "countof(exception_names)");
