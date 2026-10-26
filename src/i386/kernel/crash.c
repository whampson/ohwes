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
#include <signal.h>
#include <i386/bitops.h>
#include <i386/cpu.h>
#include <i386/debug.h>
#include <i386/interrupt.h>
#include <i386/ps2.h>
#include <i386/x86.h>
#include <kernel/kernel.h>
#include <kernel/kprint.h>
#include <kernel/console.h>
#include <kernel/irq.h>
#include <kernel/terminal.h>
#include <kernel/serial.h>
#include <kernel/vga.h>
#include <sys/ohwes.h>

#define CRASH_MSG_BUFSIZ    256
#define CRASH_MARGIN        5

// convenient ANSI escape sequence wrappers
#define BOLD(s)             ANSI_BOLD s ANSI_UNBOLD
#define RED(s)              ANSI_RED  s ANSI_FG_DEFAULT

#define MSG_FATAL_REBOOT    "The system cannot be recovered and must be restarted."

// stack dump dimensions
#define STACK_DUMP_ROWS     8
#define STACK_DUMP_COLS     4

#if DEBUG
int g_test_crashkey;
int g_test_soft_double_fault;
#endif

static const char *exception_names[NR_EXCEPTIONS];

static int console_print(const char *fmt, ...) __format_printf(1, 2);
static int vga_print(const char *fmt, ...) __format_printf(1, 2);
static int vga_vprint(const char *fmt, va_list args) __format_printf(1, 0);
static int vga_write(const char *buf, size_t count);

static void vga_print_centered(int maxwidth, const char *fmt, ...) __format_printf(2, 3);
static void vga_print_wrapped(int margin, const char *fmt, ...) __format_printf(2, 3);

extern __init void init_early_terminal(int num, struct terminal *term);
extern void __terminal_putc(struct terminal *term, char c); // write directly to terminal, bypassing TTY

typedef int (*dumpfn_t)(const char *fmt, ...) __format_printf(1, 2);

static void show_kdcom_msg(dumpfn_t dumpfn);

// all the "dump" functions print a leading newline
static void dump_cpu(struct cpu_state *cpu, dumpfn_t to);
static void dump_gprs(struct iregs *regs, dumpfn_t to);
static void dump_ctrl_regs(struct cpu_state *cpu, dumpfn_t to);
static void dump_table_regs(struct cpu_state *cpu, dumpfn_t to);
static void dump_segregs(struct cpu_state *cpu, dumpfn_t to);
static void dump_stack(struct cpu_state *cpu, dumpfn_t to, int max_rows, int num_cols);
static void dump_segsel(struct segsel *segsel, dumpfn_t to);

// ----------------------------------------------------------------------------

__noreturn void __idle(void)
{
    for (;;) {
        __sti(); __hlt();
        // TODO: schedule new task
    }
}

__noreturn void __hard_reset(void)
{
    kprint(KLOG_LEVEL_ALERT "system is going down NOW!\n");

    // first, try resetting via the PS/2 controller
    ps2_write_cmd(PS2_CMD_SYSRESET);

    // if that didn't work, triple fault
    struct table_desc idt_desc = { .limit = 0, .base = 0 };
    __lidt(idt_desc);   // yoink away the IDT :D
    __sti();

    // and if by some miracle /that/ didn't work, show a message and spin forever
    panic("unable to shut down -- please hard-reset your computer");
}

void __debug_break(void)
{
    __sti(); __int3();
}

// ----------------------------------------------------------------------------

static void show_kdcom_msg(dumpfn_t dumpfn)
{
    if (!dumpfn) {
        return;
    }

    if (dumpfn == kprint) dumpfn(KLOG_LEVEL_WARN);
    dumpfn("\e[5m*** DEBUG BREAK - attach a debugger (GDB) to COM%d now...",
        CONFIG_SERIAL_DEBUG_COM);
    if (dumpfn == kprint) dumpfn(KLOG_LEVEL_DEFAULT);
}

//
// Capture the extraneous CPU state and combine with interrupt regs into a
// cpu state object.
//
static void capture_cpu_state(struct cpu_state *state, struct iregs *iregs)
{
    memset(state, 0, sizeof(struct cpu_state));
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
    state->iregs.vec = iregs->vec;
    state->iregs.err = iregs->err;
    state->iregs.eip = iregs->eip;
    state->iregs.cs = iregs->cs;
    state->iregs.eflags = iregs->eflags;
    state->iregs.esp = iregs->esp;
    state->iregs.ss = iregs->ss;
    store_cr0(state->cr0);
    store_cr2(state->cr2);
    store_cr3(state->cr3);
    if (cpu_has_cr4()) {
        store_cr4(state->cr4);
    }
    __sgdt(state->gdtr);
    __sidt(state->idtr);
    __sldt(state->ldtr);
    __str(state->tr);
}

static __noreturn void die(struct iregs *regs, dumpfn_t dumpfn)
{
    irq_setmask(IRQ_MASKALL);

    bool has_kb = kb_avail() && regs->vec != IRQ_KEYBOARD;
    if (has_kb) {
        // TODO: disallow terminal switching
        //       sysrq probably ok, but text will mess up crash screen
        // TODO: "restrict" keyboard? (Ctrl+Alt+Del only)
        irq_unmask(IRQ_KEYBOARD);
    }

#if CONFIG_SERIAL_DEBUG
    show_kdcom_msg(dumpfn);
    if (CONFIG_SERIAL_DEBUG_COM == 1 || CONFIG_SERIAL_DEBUG_COM == 3) {
        irq_unmask(IRQ_COM1);
    }
    else {
        irq_unmask(IRQ_COM2);
    }
#endif

    for (;;) {
#if CONFIG_SERIAL_DEBUG
        __enter_debugger_loop(regs, false);
#else
        __sti(); __hlt();   // this is the end... my only friend... the end...
#endif
    }
}

#if CONFIG_SHOW_CRASH_SCREEN
static __noreturn void show_crash_screen(
    struct cpu_state *cpu,
    int color, int margin,
    const char *banner,
    const char *primary_text,
    const char *secondary_text)
{
    const int MaxWidth = vga_get_cols();
    const int MaxHeight = vga_get_rows();

    bool has_primary = (primary_text && *primary_text);
    bool has_secondary = (secondary_text && *secondary_text);
    int banner_pos = max(4, (MaxHeight / 3));

    // clear screen
    vga_print(ANSI_UNBOLD ANSI_FG_DEFAULT); // default foreground
    vga_print("\e[4%dm\e[2J", color & 7);   // paint background

    // dump regs
    int stack_max_lines = (has_secondary) ? 5 : STACK_DUMP_ROWS;
    int stack_lines = (cpu->iregs.esp != 0) ? min(stack_max_lines,  // ridiculous I know...
        max((ptrdiff_t) ((cpu->iregs.ebp - cpu->iregs.esp) >> 2) / STACK_DUMP_ROWS, 0)) : 0;
    int regs_pos = 4 + stack_lines;

    vga_print("\e[999;999H\r\e[%dA", regs_pos);
    dump_stack(cpu, vga_print, stack_max_lines, STACK_DUMP_COLS);
    vga_print("\n");
    dump_gprs(&cpu->iregs, vga_print);
    dump_ctrl_regs(cpu, vga_print);

    banner_pos = max(4,
        (MaxHeight / 3) - stack_lines -
        (STACK_DUMP_ROWS - stack_max_lines));

    // print banner
    vga_print("\e[%dH", banner_pos);
    vga_print_centered(MaxWidth, ANSI_INVERT " %s " ANSI_UNINVERT, banner);

    // print messages
    if (has_primary) {
        vga_print("\n\n\n");
        vga_print_wrapped(margin, primary_text);
    }
    if (has_secondary) {
        vga_print("\n\n");
        vga_print_wrapped(margin, secondary_text);
    }

    // prompt
    vga_print("\n\n\n");

#if CONFIG_SERIAL_DEBUG
    vga_print_centered(MaxWidth, "\e4\e[5m*** COMDBG AVAILABLE ***\e[25m");
    vga_print("\n");
    vga_print_centered(MaxWidth,
        "Attach a debugger (GDB) to COM%d now . . .",
        CONFIG_SERIAL_DEBUG_COM);
#else
    if (kb_avail() && cpu->iregs.vec != IRQ_KEYBOARD) {
        vga_print_centered(MaxWidth,
            "Press CTRL+ALT+DEL to restart your computer . . . \e6");
    }
    else {
        vga_print_centered(MaxWidth,
            "The exception occurred in the keyboard handler.\n");
        vga_print_centered(MaxWidth,
            "Please restart your computer manually.\e6");
    }
#endif

    // idle forever or wait in debugger loop
    die(&cpu->iregs, NULL); // already printed COMDBG msg
}
#endif // CONFIG_SHOW_CRASH_SCREEN

//
// Uh oh! An exception occurred in the exception handler.
// Do the bare minimum here to show diagnostic information to the user.
//
// This is to be called ONLY by handle_exception() if we were previously
// handling an exception!
//
static __noreturn void handle_soft_double_fault(
    struct cpu_state *curr_cpu, struct cpu_state *prev_cpu)
{
    console_print(ANSI_FG_DEFAULT ANSI_BOLD "\n(1) %s at %p",
        exception_names[curr_cpu->iregs.vec], _P(curr_cpu->iregs.eip));
    dump_cpu(curr_cpu, console_print);
    dump_stack(curr_cpu, console_print, STACK_DUMP_ROWS, STACK_DUMP_COLS);
    console_print("\n\n(2) %s at %p",
        exception_names[prev_cpu->iregs.vec], _P(prev_cpu->iregs.eip));
    dump_cpu(prev_cpu, console_print);
    dump_stack(prev_cpu, console_print, STACK_DUMP_ROWS, STACK_DUMP_COLS);
    console_print("\n\n" RED("*** FATAL: %s (1) occurred while handling %s (2)"),
        exception_names[curr_cpu->iregs.vec], exception_names[prev_cpu->iregs.vec]);
    console_print(ANSI_UNBOLD ANSI_FG_DEFAULT);

#if CONFIG_SHOW_CRASH_SCREEN
    char msgbuf[CRASH_MSG_BUFSIZ];
    snprintf(msgbuf, sizeof(msgbuf),
        "A fatal exception %02lX (%s) occurred at %p while handling a previous "
        "%s exception %02lX at %p. " MSG_FATAL_REBOOT,
        curr_cpu->iregs.vec, exception_names[curr_cpu->iregs.vec], _P(curr_cpu->iregs.eip),
        exception_names[prev_cpu->iregs.vec], prev_cpu->iregs.vec, _P(prev_cpu->iregs.eip));
    show_crash_screen(curr_cpu, SGR_RED, 5, "Double Fault", msgbuf, NULL);
#else
    die(&curr_cpu->iregs, console_print);
#endif
}

//
// Generic x86 exception handler.
//
__fastcall void handle_exception(struct iregs *iregs)
{
    // static vars for soft double-fault detection
    static volatile uint32_t s_crashing = false;
    static struct cpu_state prev_cpu;

    char errbuf[CRASH_MSG_BUFSIZ];
    struct cpu_state cpu;
    size_t errbuf_len;

#if CONFIG_SERIAL_DEBUG
    if (iregs->vec == BREAKPOINT || iregs->vec == DEBUG_EXCEPTION) {
        show_kdcom_msg((g_consoles) ? kprint : vga_print);
        __enter_debugger_loop(iregs, /*from_com=*/false);    // blocks until a debugger connects
        return;
    }
#endif

    // get the remaining regs
    capture_cpu_state(&cpu, iregs);

    // if we're already crashing... well... that's not good... handle it here!
    if (test_and_set_bit(&s_crashing, 0)) {
        handle_soft_double_fault(&cpu, &prev_cpu);  // noreturn
    }
    prev_cpu = cpu;

#if DEBUG && CONFIG_CRASH_KEY
    // test a software double fault
    if (g_test_soft_double_fault) {
        __asm__ volatile (".short 0x0A0F");
    }
#endif

    // collect error info
    if (iregs->vec == PAGE_FAULT) {
        int us = iregs->err & PF_US;
        int wr = iregs->err & PF_WR;
        int p = iregs->err & PF_P;
        int rsvd = iregs->err & PF_RSVD;
        errbuf_len = snprintf(errbuf, CRASH_MSG_BUFSIZ, "%s caused by %s%s%p.",
            (p) ? ((rsvd) ? "Reserved bit violation" : "Privilege violation")
                : "Access violation",
            (us) ? "user mode " : "kernel mode ",
            (wr) ? "write to " : "read from ",
            _P(cpu.cr2));
    }
    else if (iregs->err) {
        errbuf_len = snprintf(errbuf, sizeof(errbuf), "The issue occurred in %s(%02lX)%s.",
            (iregs->err & ERR_IDT) ? "IDT" :
                (iregs->err & ERR_TI) ? "LDT" : "GDT",
            (iregs->err & ERR_INDEX) >> 3,
            (iregs->err & ERR_EXT) ? " and originated via interrupt" : "");
    }
    else {
        errbuf_len = snprintf(errbuf, sizeof(errbuf), "%s", "");    // ensure NUL written
    }

    // print it all to the console
    console_print(ANSI_FG_DEFAULT ANSI_BOLD "\n");
    dump_cpu(&cpu, console_print);
    dump_stack(&cpu, console_print, STACK_DUMP_ROWS, STACK_DUMP_COLS);
    console_print("\n\n"
        RED("*** FATAL: %s exception %02lX occurred at %p"),
        exception_names[iregs->vec], iregs->vec, _P(iregs->eip));
    if (errbuf_len) {
        console_print("\n%s", errbuf);
    }
    console_print(ANSI_UNBOLD ANSI_FG_DEFAULT);


#if CONFIG_SHOW_CRASH_SCREEN
    char msgbuf[CRASH_MSG_BUFSIZ];
    snprintf(msgbuf, sizeof(msgbuf),
        "A fatal exception %02lX (%s) occurred at %p. " MSG_FATAL_REBOOT,
        iregs->vec, exception_names[iregs->vec], _P(iregs->eip));
    show_crash_screen(&cpu, SGR_BLUE, CRASH_MARGIN, OS_NAME, msgbuf, errbuf);
#else
    die(&cpu.iregs, console_print);
#endif
}

__fastcall __noreturn void _kpanic(const char *fmt, ...)
{
    typedef int (*vdumpfn_t)(const char *fmt, va_list args);

    va_list args;
    dumpfn_t dump;
    vdumpfn_t vdump;
    const char *prefix, *suffix;

    dump = (g_consoles) ? kprint  : vga_print;
    vdump = (g_consoles) ? vkprint : vga_vprint;
    prefix = (g_consoles) ? KLOG_LEVEL_FATAL : ANSI_BOLD ANSI_RED;
    suffix = (g_consoles) ? KLOG_LEVEL_DEFAULT : ANSI_UNBOLD ANSI_FG_DEFAULT;

    va_start(args, fmt);
    dump(prefix);
    vdump(fmt, args);
    dump(suffix);
    va_end(args);

    // TODO: stack trace

    for (;;) {
        __debug_break();    // TODO: allow continue?
    }
}

static void dump_cpu(struct cpu_state *cpu, dumpfn_t dump)
{
    dump_gprs(&cpu->iregs, dump);
    dump_ctrl_regs(cpu, dump);
    dump_table_regs(cpu, vga_print);
    dump_segregs(cpu, dump);
}

static void dump_gprs(struct iregs *regs, dumpfn_t dump)
{
    struct eflags *flags = (struct eflags *) &regs->eflags;

    if (regs->err) {
        dump("\nERR=%08lX", regs->err);
    }

    dump("\nEAX=%08lX EBX=%08lX ECX=%08lX EDX=%08lX",
        regs->eax, regs->ebx, regs->ecx, regs->edx);
    dump("\nESI=%08lX EDI=%08lX ESP=%08lX EBP=%08lX",
        regs->esi, regs->edi, regs->esp, regs->ebp);
    dump("\nEIP=%08lX ", regs->eip);

    dump("EFL=%08lx [", flags->_value);
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

static void dump_ctrl_regs(struct cpu_state *cpu, dumpfn_t dump)
{
    dump("\nCR0=%08lX CR2=%08lX CR3=%08lX CR4=%08lX",
        cpu->cr0, cpu->cr2, cpu->cr3, cpu->cr4);
}

static void dump_table_regs(struct cpu_state *cpu, dumpfn_t dump)
{
    struct table_desc *gdt_desc = (struct table_desc *) &cpu->gdtr;
    struct table_desc *idt_desc = (struct table_desc *) &cpu->idtr;

    dump("\nGDTR=%08lX,%05X", gdt_desc->base, gdt_desc->limit);
    dump("\nIDTR=%08lX,%05X", idt_desc->base, idt_desc->limit);
    dump("\nLDTR="); dump_segsel((struct segsel *) &cpu->ldtr, dump);
    dump("\nTR="); dump_segsel((struct segsel *) &cpu->tr, dump);
}

static void dump_segregs(struct cpu_state *cpu, dumpfn_t dump)
{
    dump("\nSS="); dump_segsel((struct segsel *) &cpu->iregs.ss, dump);
    dump("\nCS="); dump_segsel((struct segsel *) &cpu->iregs.cs, dump);
    dump("\nDS="); dump_segsel((struct segsel *) &cpu->iregs.ds, dump);
    dump("\nES="); dump_segsel((struct segsel *) &cpu->iregs.es, dump);
    dump("\nFS="); dump_segsel((struct segsel *) &cpu->iregs.fs, dump);
    dump("\nGS="); dump_segsel((struct segsel *) &cpu->iregs.gs, dump);
}

static void dump_stack(struct cpu_state *cpu, dumpfn_t dump, int max_rows, int num_cols)
{
    const uint32_t *esp = (const uint32_t *) cpu->iregs.esp;
    const uint32_t *ebp = (const uint32_t *) cpu->iregs.ebp;

    for (int i = 0; i < max_rows
            && ((uint32_t) esp % PAGE_SIZE) != 0
            && esp < ebp; i++)
    {
        dump("\n%p:", esp);
        for (int k = 0; k < num_cols
                && ((uint32_t) esp % PAGE_SIZE) != 0
                && esp < ebp;
            k++, esp++)
        {
            dump(" %08lX", *((uint32_t *) esp));
        }
    }
}

static void dump_segsel(struct segsel *segsel, dumpfn_t dump)
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

// print message to all attached consoles,
//   like kprint but with no klog features,
//   will write directly to vga terminal if no console is registered
static int console_print(const char *fmt, ...)
{
    char buf[CRASH_MSG_BUFSIZ] = { };
    va_list args;
    size_t count;
    struct console *cons;

    va_start(args, fmt);
    count = vsnprintf(buf, CRASH_MSG_BUFSIZ, fmt, args);
    va_end(args);

    if (!g_consoles) {
        return vga_write(buf, count);
    }
    for (cons = g_consoles; cons; cons = cons->next) {
        cons->write(cons, buf, count);
    }
    return count;
}

// print directly to the VGA via terminal
static int vga_print(const char *fmt, ...)
{
    va_list args;
    size_t count;

    va_start(args, fmt);
    count = vga_vprint(fmt, args);
    va_end(args);

    return count;
}

static int vga_vprint(const char *fmt, va_list args)
{
    size_t count;
    char buf[CRASH_MSG_BUFSIZ] = { };

    count = vsnprintf(buf, CRASH_MSG_BUFSIZ, fmt, args);
    return vga_write(buf, count);
}

static int vga_write(const char *buf, size_t count)
{
    struct terminal *term;
    const char *p;

    term = get_terminal(CONFIG_VT_CONSOLE_NUM);
    if (!term->initialized) {
        init_early_terminal(CONFIG_VT_CONSOLE_NUM, term);
    }

    p = buf;
    while (p < buf + count) {
        if (*p == '\n') {
            __terminal_putc(term, '\r');
        }
        __terminal_putc(term, *p++);
    }

    return (p - buf);
}

static void vga_print_centered(int maxwidth, const char *fmt, ...)
{
    int len;
    int col;
    char c;
    int esc;
    const char *p;
    char buf[CRASH_MSG_BUFSIZ];
    va_list args;

    // TODO: handle multiple lines
    //       handle wrap

    if (maxwidth < 0) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(buf, CRASH_MSG_BUFSIZ, fmt, args);
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

    vga_print("\e[%dG%s", col, buf);
}

static void vga_print_wrapped(int margin, const char *fmt, ...)
{
    const int MaxWidth = vga_get_cols();

    // technically "wrap and left justify within margin"

    va_list args;
    char buf[CRASH_MSG_BUFSIZ];
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
    vsnprintf(buf, CRASH_MSG_BUFSIZ, fmt, args);
    va_end(args);

    esc = 0;
    esclen = 0;
    wordlen = 0;
    linelen = 2 * margin;
    print_margin = true;

    p = buf;
    word = p;

    while ((p - buf) < CRASH_MSG_BUFSIZ && *p != '\0') {
        // find end of word by looking for space
        word = p;
        wordlen = 0;
        esclen = 0;
        for (; (p - buf) < CRASH_MSG_BUFSIZ && *p != '\0'; p++) {
            if (linelen > MaxWidth) {
                vga_print("\n");
                linelen = (2 * margin) + wordlen;
                print_margin = true;
            }

            // TODO: handle tabs?

            if (*p == '\n') {
                vga_print("%.*s", wordlen + esclen, word);
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
                    vga_print(" ");
                }
                print_margin = false;
            }

            if (isspace(*p)) {
                vga_print("%.*s", wordlen + esclen, word);
                break;
            }

            wordlen++;
            linelen++;
        }

        // eat up trailing spaces
        for (; isspace(*p) && *p != '\0'; p++) {
            if (linelen > MaxWidth) {
                vga_print("\n");
                linelen = (2 * margin) + wordlen;
                print_margin = true;
                break;
            }

            if (linelen > 0) {
                linelen++;
                vga_print("%c", *p);
            }
        }
    }
    vga_print("%.*s", wordlen + esclen, word);
}

#if DEBUG && CONFIG_CRASH_KEY
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
            __debug_break();
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
        case 7:     // F7 - kernel stack page fault
            __asm__ volatile ("movl $0, %esp; popl %eax");
            break;

        // case 7:     // F7 - spurious interrupt
        //     __asm__ volatile ("int $0x27");
        //     break;

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
            pr_error("soft double fault...\n");
            g_test_soft_double_fault = true;
            __asm__ volatile ("idiv %0" :: "a"(0), "b"(0));
            break;
        }
        case 11: {  // F11 - true double fault
            pr_error("double fault...\n");
            volatile struct x86_desc *idt;
            idt = get_idt();
            idt[BREAKPOINT].trap.p = 0;
            idt[SEGMENT_NOT_PRESENT].trap.p = 0;
            __asm__ volatile("int3");
            break;
        }
        case 12: {  // F12 - triple fault
            pr_error("triple fault...\n");
            struct table_desc idt_desc = { .limit = 0, .base = 0 };
            __lidt(idt_desc);   // yoink away the IDT :D
            break;
        }
    }

    g_test_crashkey = 0;
}
#endif

static const char *exception_names[NR_EXCEPTIONS] =
{
    /*0x00*/ "DIVIDE_ERROR",
    /*0x01*/ "DEBUG_TRAP",
    /*0x02*/ "NMI_INTERRUPT",
    /*0x03*/ "BREAKPOINT",
    /*0x04*/ "OVERFLOW",
    /*0x05*/ "BOUND_RANGE",
    /*0x06*/ "INVALID_OPCODE",
    /*0x07*/ "DEVICE_NOT_AVAILABLE",
    /*0x08*/ "DOUBLE_FAULT",
    /*0x09*/ "SEGMENT_OVERRUN",
    /*0x0A*/ "INVALID_TSS",
    /*0x0B*/ "SEGMENT_NOT_PRESENT",
    /*0x0C*/ "STACK_FAULT",
    /*0x0D*/ "PROTECTION_FAULT",
    /*0x0E*/ "PAGE_FAULT",
    /*0x0F*/ "UNBOUND_0F",
    /*0x10*/ "MATH_FAULT",
    /*0x11*/ "ALIGNMENT_CHECK",
    /*0x12*/ "MACHINE_CHECK",
    /*0x13*/ "SIMD_FAULT",
    /*0x14*/ "UNBOUND_14",
    /*0x15*/ "UNBOUND_15",
    /*0x16*/ "UNBOUND_16",
    /*0x17*/ "UNBOUND_17",
    /*0x18*/ "UNBOUND_18",
    /*0x19*/ "UNBOUND_19",
    /*0x1A*/ "UNBOUND_1A",
    /*0x1B*/ "UNBOUND_1B",
    /*0x1C*/ "UNBOUND_1C",
    /*0x1D*/ "UNBOUND_1D",
    /*0x1E*/ "UNBOUND_1E",
    /*0x1F*/ "UNBOUND_1F",
};
static_assert(
    countof(exception_names) == NR_EXCEPTIONS, "countof(exception_names)");
