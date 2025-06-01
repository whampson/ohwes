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

#include <stdarg.h>
#include <i386/cpu.h>
#include <i386/interrupt.h>
#include <i386/x86.h>
#include <kernel/kernel.h>
#include <kernel/ohwes.h>
#include <kernel/irq.h>

#define CRASH_PRINT_BUFSIZ  128

// convenient ANSI escape sequence wrappers
#define BOLD(s)             "\e[1m" s "\e[22m"
#define ITALIC(s)           "\e[3m" s "\e[23m"
#define UNDERLINE(s)        "\e[4m" s "\e[24m"

// optional visual information
#define DUMP_SEGMENT_REGS   1
#define DUMP_STACK          1

// stack dump dimensions
#define STACK_ROWS          8
#define STACK_COLUMNS       4

#define __early __attribute__((section(".data")))

#if DEBUG
int g_test_crashkey = 0;
int g_test_soft_double_fault = 0;
#endif

extern bool g_kb_initialized;
extern bool g_timer_initialized;

extern struct console *g_consoles;

static const char *exception_names[NR_EXCEPTIONS];

struct x86_regs {
    struct iregs iregs;
    uint32_t cr0;
    uint32_t cr2;
    uint32_t cr3;
    uint32_t cr4;
};

static void cprint(const char *fmt, ...);

static void dump_exception(struct x86_regs *regs);
static void dump_regs(struct x86_regs *regs);
static void print_segsel(struct segsel *segsel);
static void print_eflags(struct eflags *flags);

static __noreturn __fastcall
void handle_soft_double_fault(struct x86_regs *regs, struct x86_regs *orig_regs);

//
// Generic x86 exception handler.
//
__fastcall void handle_exception(struct iregs *iregs)
{
    // static vars for soft double-fault detection
    __early static bool crashing = false;
    __early static struct x86_regs orig_regs;

    // capture ALL the regs
    struct x86_regs regs;
    store_cr0(regs.cr0);
    store_cr2(regs.cr2);
    store_cr3(regs.cr3);
    regs.cr4 = 0;
    if (cpu_has_cr4()) {
        store_cr4(regs.cr4);
    }
    regs.iregs.ebx = iregs->ebx;
    regs.iregs.ecx = iregs->ecx;
    regs.iregs.edx = iregs->edx;
    regs.iregs.esi = iregs->esi;
    regs.iregs.edi = iregs->edi;
    regs.iregs.ebp = iregs->ebp;
    regs.iregs.eax = iregs->eax;
    regs.iregs.ds  = iregs->ds;
    regs.iregs.es  = iregs->es;
    regs.iregs.fs  = iregs->fs;
    regs.iregs.gs  = iregs->gs;
    regs.iregs.vec_num = iregs->vec_num;
    regs.iregs.err_code = iregs->err_code;
    regs.iregs.eip = iregs->eip;
    regs.iregs.cs  = iregs->cs;
    regs.iregs.eflags = iregs->eflags;
    if (iregs->vec_num == EXCEPTION_DF) {
        // Double Fault exceptions are the only type of exception handled with
        // a task gate, where the full context if the interrupted program is
        // known regardless of privilege level. Therefore, we know ESP and SS
        // are valid and don't need to do fancy x86 wizardry to determine where
        // to find them.
        regs.iregs.esp = iregs->esp;
        regs.iregs.ss = iregs->ss;
    }
    else {
        regs.iregs.esp = get_esp(iregs);
        regs.iregs.ss = get_ss(iregs);
    }

    // if we're already crashing... well... that's not good...
    // handle it here!
    if (crashing) {
        handle_soft_double_fault(&regs, &orig_regs);
    }
    crashing = true;
    orig_regs = regs;

#if DEBUG
    // test a software double fault
    if (g_test_soft_double_fault) {
        __asm__ volatile (".short 0x0A0F");
    }
#endif

    // TODO: handle recoverable exceptions like Page Faults

    // dump info to the console
    cprint("\n\e[1;31mfatal: ");
    dump_exception(&regs);

    // TODO: show a crash screen

    for (;;);
    // crashing = false;   // if we ever return...
}

//
// x86 Double Fault exception handler. An exception occurred within the CPU
// while handling a different exception. Not to be called directly.
//
// The IDT is set up to perform a task switch if a Double Fault exception
// occurs (task gate). This is to ensure we end up with a known good stack
// so we can print diagnostic information to the user. Grab the program
// context from the faulting program's TSS and feed it to
// handle_exception().
//
__noreturn void handle_double_fault(void)
{
    struct tss *tss = get_curr_tss();
    struct tss *fault_tss = get_tss(tss->prev_task);

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
    regs.vec_num = EXCEPTION_DF;
    regs.err_code = 0;
    regs.eip = fault_tss->eip;
    regs.cs  = fault_tss->cs;
    regs.eflags = fault_tss->eflags;
    regs.esp = fault_tss->esp;
    regs.ss  = fault_tss->ss;

    handle_exception(&regs);
    for (;;);
}

//
// Software double fault. An exception occurred in the exception handler.
// Do the bare minimum here to show diagnostic information to the user.
//
// This is to be called ONLY by handle_exception() if we were previously
// handling an exception!
//
__noreturn __fastcall void handle_soft_double_fault(
    struct x86_regs *regs, struct x86_regs *orig_regs)
{
    cprint("\n\e[1;31mfatal: ");
    dump_exception(regs);
    cprint("\n\e[1;31m(occurred while handling) ");
    dump_exception(orig_regs);
    cprint("\n\n");
    cprint("\e[1;31mfatal: software double fault, your system is toast!\e[0m");

    for (;;);
}

static void dump_exception(struct x86_regs *regs)
{
    const uint32_t *esp = (const uint32_t *) regs->iregs.esp;
    const uint32_t *ebp = (const uint32_t *) regs->iregs.ebp;

    cprint("%s exception at %04X:%08X\e[0m\n",
        exception_names[regs->iregs.vec_num], regs->iregs.cs, regs->iregs.eip);
    dump_regs(regs);

#if DUMP_STACK
    for (int i = 0; i < STACK_ROWS
            && ((uint32_t) esp % PAGE_SIZE) != 0
            && esp < ebp;
        i++)
    {
        cprint("\n%08X:", esp);
        for (int k = 0; k < STACK_COLUMNS
                && ((uint32_t) esp % PAGE_SIZE) != 0
                && esp < ebp;
            k++, esp++)
        {
            cprint(" %08X", *((uint32_t *) esp));
        }
    }
#endif

    cprint("\e[0m");
}

static void dump_regs(struct x86_regs *regs)
{
    cprint("EAX=%08X EBX=%08X ECX=%08X EDX=%08X",
        regs->iregs.eax, regs->iregs.ebx, regs->iregs.ecx, regs->iregs.edx);
    cprint("\nESI=%08X EDI=%08X ESP=%08X EBP=%08X",
        regs->iregs.esi, regs->iregs.edi, regs->iregs.esp, regs->iregs.ebp);
    cprint("\nCR0=%08X CR2=%08X CR3=%08X CR4=%08X",
        regs->cr0, regs->cr2, regs->cr3, regs->cr4);
    cprint("\nEIP=%08X ERR=%08X ", regs->iregs.eip, regs->iregs.err_code);
    print_eflags((struct eflags *) &regs->iregs.eflags);

#if DUMP_SEGMENT_REGS
    cprint("\nSS="); print_segsel((struct segsel *) &regs->iregs.ss);
    cprint("\nCS="); print_segsel((struct segsel *) &regs->iregs.cs);
    cprint("\nDS="); print_segsel((struct segsel *) &regs->iregs.ds);
    cprint("\nES="); print_segsel((struct segsel *) &regs->iregs.es);
    cprint("\nFS="); print_segsel((struct segsel *) &regs->iregs.fs);
    cprint("\nGS="); print_segsel((struct segsel *) &regs->iregs.gs);
#endif
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

static void print_eflags(struct eflags *flags)
{
    cprint("[");
    if (flags->id)   cprint(" ID");
    // if (flags->vip)  cprint(" VIP");
    // if (flags->vif)  cprint(" VIF");
    if (flags->ac)   cprint(" AC");
    // if (flags->vm)   cprint(" VM");
    if (flags->rf)   cprint(" RF");
    if (flags->nt)   cprint(" NT");
    cprint(" IOPL=%d", flags->iopl);
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

// like kprint but with a smaller stack buffer
void cprint(const char *fmt, ...)
{
    char buf[CRASH_PRINT_BUFSIZ+1] = { };
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, CRASH_PRINT_BUFSIZ, fmt, args);
    va_end(args);

    if (has_console()) {
        console_print(buf);
    }
    else {
        // TODO: print directly to frame buffer
    }
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
            volatile struct table_desc idt_desc = { };
            __sidt(idt_desc);
            idt = (volatile struct x86_desc *) idt_desc.base;
            idt[EXCEPTION_DE].trap.p = 0;   // remove divide error trap (1)
            idt[EXCEPTION_NP].trap.p = 0;   // remove seg not present trap (2)
            __asm__ volatile ("idiv %0" :: "a"(0), "b"(0));
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
    /*0x00*/ "DIVIDE_BY_ZERO",
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
static_assert(countof(exception_names) == NR_EXCEPTIONS,
    "countof(exception_names)");
