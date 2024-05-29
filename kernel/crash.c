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

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <ohwes.h>
#include <console.h>
#include <interrupt.h>
#include <irq.h>
#include <io.h>
#include <x86.h>
#include <cpu.h>
#include <fs.h>

#define CRASH_COLOR     CONSOLE_BLUE
#define PANIC_COLOR     CONSOLE_BLUE
#define IRQ_COLOR       CONSOLE_RED
#define NMI_COLOR       CONSOLE_RED
#define BANNER_COLOR    CONSOLE_BLUE
#define CRASH_BANNER    " " OS_NAME " "
#define CRASH_WIDTH     80
#define CRASH_BUFSIZ    256

static const char *exception_names[NUM_EXCEPTIONS];

static void center_text(const char *str, ...);
static void print_flags(uint32_t eflags);
static void print_segsel(int segsel);
static void print_banner(const char *banner);
static void crash_print(const char *fmt, ...);
static void interrupt_crash(struct iregs *regs);

extern int console_read(struct file *file, char *buf, size_t count);
extern int console_write(struct file *file, const char *buf, size_t count);

#define kbflush()   while (console_read(NULL, &c, 1) != 0) { }
#define kbhit()     while (console_read(NULL, &c, 1) == 0) { }
#define kbwait()    ({ kbflush(); kbhit(); })

#define BRIGHT(s)   "\e[1m" s "\e[22m"

__fastcall
void crash(struct iregs *regs)
{
    char c;
    uint16_t _cs;
    struct segsel *curr_cs;
    struct segsel *fault_cs;
    int curr_pl, fault_pl;
    bool pl_change;
    uint32_t cr0, cr2, cr3;
    uint32_t *stack_ptr;
    uint16_t irq_mask;
    bool irq;
    bool nmi;
    bool pf;

    // grab the control registers
    read_cr0(cr0);
    read_cr2(cr2);
    read_cr3(cr3);

    // get the current and faulting privilege levels
    read_cs(_cs);
    curr_cs = (struct segsel *) &_cs;
    fault_cs = (struct segsel *) &regs->cs;
    curr_pl = curr_cs->rpl;
    fault_pl = fault_cs->rpl;

    // did we change privilege levels?
    pl_change = false;
    if (curr_pl != fault_pl) {
        pl_change = true;
    }

    // enable select interrupts
    irq_mask = irq_getmask();
    irq_setmask(0xFFFF);
    irq_unmask(IRQ_KEYBOARD);
    irq_unmask(IRQ_TIMER);
    sti();
    // TODO: should probably check whether we crashed from the keyboard or timer
    // ISR before deciding to enable those interrupts. Also the console_write()
    // function, because if we crashed there we're SOL here...

    // locate faulting stack
    if (pl_change) {
        stack_ptr = (uint32_t *) regs->esp;
    }
    else {
        // SS and ESP are not pushed onto the stack if an interrupt did not
        // change privilege levels, i.e. we are using the same stack. Our common
        // interrupt handler pushed iregs into the stack, so we must subtract
        // (or add, because Intel) the size of the iregs structure to the
        // current iregs pointer, less ESP and SS, in order to get the top of
        // the faulting function's stack.
        stack_ptr = (uint32_t *) ((uint32_t) regs + SIZEOF_IREGS_NO_PL_CHANGE);
    }


    // layout tuning params
    const int banner_line = 5;
    const int irq_banner_line = 9;
    const int regs_line = 17;
    const int seg_regs_line = 18;
    const int seg_regs_col = 26;
    const int stack_num_lines = 8;
    const int stack_width_dwords = 2;
    const int stack_left_col = VGA_COLS - (9 + (stack_width_dwords * 9));

    // was it a device interrupt?
    irq = (regs->vec_num < 0);

    // or a non-maskable interrupt?
    nmi = (regs->vec_num == EXCEPTION_NMI);

    // or a page fault?
    pf = (regs->vec_num == EXCEPTION_PF);

    if (irq || nmi) {
        crash_print("\e[0;0H\e[37;4%dm\e[2J\e5", IRQ_COLOR);
        crash_print("\e[%d;0H", irq_banner_line);

        if (irq) {
            print_banner(" Unhandled Interrupt ");
            crash_print("\n\n\e[37;4%dm", IRQ_COLOR);
            center_text("An unhandled device interrupt was raised by " BRIGHT("IRQ %d") ".",
                ~regs->vec_num, regs->cs, regs->eip);
        }
        else {
            print_banner(" Non-Maskable Interrupt ");
            crash_print("\n\n\e[37;4%dm", NMI_COLOR);
            crash_print("\tA non-maskable interrupt was raised. If this continues, press");
            crash_print("\n\tCtrl+Alt+Del to restart your computer.");
            if (inb(SYSCNTL_PORT_A) & 0x10) {
                crash_print("\n\n\t * Watchdog Timer");
            }
            if (inb(SYSCNTL_PORT_B) & 0x40) {
                center_text("\n\n\t * Channel Check");
            }
            if (inb(SYSCNTL_PORT_B) & 0x80) {
                center_text("\n\n\t * Parity Check");
            }
        }
        crash_print("\n\n\n");
        center_text("Press any key to continue \e6");
        goto done;
    }

    crash_print("\e[0;0H\e[37;4%dm\e[2J\e5", CRASH_COLOR);
    crash_print("\e[%d;0H", banner_line);
    print_banner(" Fatal Exception ");
    crash_print("\n\n\e[37;4%dm", CRASH_COLOR);
    crash_print("\tA fatal exception " BRIGHT("%02X") " has occurred at " BRIGHT("%04X:%08X") ". The program",
        regs->vec_num, regs->cs, regs->eip);
    crash_print("\n\tmay be able to continue execution. Press any key to continue or");
    crash_print("\n\tpress Ctrl+Alt+Del to restart your computer.");
    crash_print("\n");
    crash_print("\n\t * Exception Name: " BRIGHT("%s"), exception_names[regs->vec_num]);
    if (regs->err_code && !pf) {
        crash_print("\n\t * Faulting Descriptor: " BRIGHT("%s(%02X)%s"),
            (regs->err_code & 0x02) ? "IDT" :
                (regs->err_code & 0x04) ? "LDT" : "GDT",
            (regs->err_code & 0xFFFF) >> 3,
            (regs->err_code & 0x01) ? " (external)" : "");
    }
    else if (pf) {
        crash_print("\n\t * Details:");
        crash_print("\n\t    - %s %s Access Violation",
            regs->err_code & PF_US ? "User" : "Supervisor",
            regs->err_code & PF_WR ? "Write" : "Read");
        crash_print("\n\t    - Linear Address: %08X", cr2);
        if (!(regs->err_code & PF_P)) {
            crash_print("\n\t    - Page Not Present");
        }
        if (regs->err_code & PF_ID) {
            crash_print("\n\t    - Instruction Fetch Page Fault");
        }
        if (regs->err_code & PF_RSVD) {
            crash_print("\n\t    - Reserved Bit Violation");
        }
        if (regs->err_code & 0xFFFFFFE0) {
            crash_print("\n\t    - NOTE: Additional Error Code Bits Set\n");
        }
    }

    // dump control registers
    crash_print("\e[%d;0H", regs_line);
    crash_print("\n CR0=%08X ", cr0);
    crash_print("\n CR2=%08X CR3=%08X", cr2, cr3);

    // dump context registers and error code
    crash_print("\n EAX=%08X EBX=%08X\n ECX=%08X EDX=%08X",
        regs->eax, regs->ebx, regs->ecx, regs->edx);
    crash_print("\n EDI=%08X ESI=%08X\n EBP=%08X ESP=%08X",
        regs->edi, regs->esi, regs->ebp, stack_ptr);
    crash_print("\n EIP=%08X ERR=%08X", regs->eip, regs->err_code);
    crash_print("\n");
    print_flags(regs->eflags);

    // dump segment registers
    crash_print("\e[%d;%dH", seg_regs_line, seg_regs_col);
    crash_print("\n\e[%dC CS=", seg_regs_col); print_segsel(regs->cs);
    crash_print("\n\e[%dC DS=", seg_regs_col); print_segsel(regs->ds);
    crash_print("\n\e[%dC ES=", seg_regs_col); print_segsel(regs->es);
    crash_print("\n\e[%dC FS=", seg_regs_col); print_segsel(regs->fs);
    crash_print("\n\e[%dC GS=", seg_regs_col); print_segsel(regs->gs);
    if (pl_change) {
        crash_print("\n\e[%dC SS=", seg_regs_col); print_segsel(regs->ss & 0xFFFF);
    }

    // dump stack
    for (int l = 0; l < stack_num_lines; l++) {
        crash_print("\e[%d;%dH",
            VGA_ROWS - stack_num_lines + l + 1, stack_left_col);
        crash_print("%08X:", stack_ptr);
        for (int w = 0; w < stack_width_dwords; w++, stack_ptr++) {
            crash_print(" %08X", *stack_ptr);
        }
    }

done:
    kbwait();
    crash_print("\e[0;0H\e[37;40m\e[2J\e5");    // restore console
    cli();
    irq_setmask(irq_mask);
    return;
}

__noreturn
void kpanic(const char *fmt, ...)
{
    va_list args;
    char buf[CRASH_BUFSIZ];

    va_start(args, fmt);
    // kprint("panic: ");
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    const int banner_line = 8;

    crash_print("\e[0;0H\e[37;4%dm\e[2J\e5", PANIC_COLOR);
    crash_print("\e[%d;0H", banner_line);
    print_banner(" Kernel Panic ");
    crash_print("\n\n\e[37;4%dm", PANIC_COLOR);
    crash_print(buf);
    die();
}

static void center_text(const char *str, ...)
{
    va_list args;
    char buf[CRASH_WIDTH];
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

        assert(esc == 0);
        len++;
    }

    col = (CRASH_WIDTH - len) / 2;
    if (col < 0) {
        col = 0;
    }
    crash_print("\e[%dG%s", col, buf);
}

static void print_segsel(int segsel)
{
    struct segsel *ss = (struct segsel *) &segsel;
    // struct seg_desc *desc = &get_seg_desc(segsel)->seg;

    crash_print("%02X(%04X|%d|%d)", // ,%X,%X%c
        segsel, ss->index, ss->ti, ss->rpl/*,
        (desc->basehi << 24) | desc->baselo,
        (desc->limithi << 16) | desc->limitlo,
        (desc->g) ? ' ' : 'b'*/);
}

static void print_flags(uint32_t eflags)
{
    struct eflags *flags = (struct eflags *) &eflags;
    crash_print(" EFL=%08X", eflags);
    crash_print(" [");
    if (flags->id)   crash_print(" ID");
    if (flags->vip)  crash_print(" VIP");
    if (flags->vif)  crash_print(" VIF");
    if (flags->ac)   crash_print(" AC");
    if (flags->vm)   crash_print(" VM");
    if (flags->rf)   crash_print(" RF");
    if (flags->nt)   crash_print(" NT");
    if (flags->of)   crash_print(" OF");
    if (flags->df)   crash_print(" DF");
    if (flags->intf) crash_print(" IF");
    if (flags->tf)   crash_print(" TF");
    if (flags->sf)   crash_print(" SF");
    if (flags->zf)   crash_print(" ZF");
    if (flags->af)   crash_print(" AF");
    if (flags->pf)   crash_print(" PF");
    if (flags->cf)   crash_print(" CF");
    crash_print(" ]");
}

static void print_banner(const char *banner)
{
    crash_print("\e[47;3%dm", BANNER_COLOR);
    center_text(banner);
    crash_print("\e[37;4%dm", BANNER_COLOR);
}

static void crash_print(const char *fmt, ...)
{
    va_list args;
    char buf[CRASH_BUFSIZ];
    size_t count;

    va_start(args, fmt);
    count = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    console_write(NULL, buf, count);
}

static const char *exception_names[NUM_EXCEPTIONS] =
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
static_assert(countof(exception_names) == NUM_EXCEPTIONS, "Bad exception_names length");
