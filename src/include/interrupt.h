/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *         File: include/interrupt.h
 *      Created: January 6, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#ifndef __ASSEMBLER__
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif

/**
 * Device IRQ numbers.
 * NOTE: these are NOT interrupt vector numbers!
 */
#define IRQ_TIMER           0       // Programmable Interval Timer (PIT)
#define IRQ_KEYBOARD        1       // PS/2 Keyboard
#define IRQ_SLAVE           2       // Slave PIC cascade signal
#define IRQ_COM2            3       // Serial Port #2
#define IRQ_COM1            4       // Serial Port #1
#define IRQ_LPT2            5       // Parallel Port #2
#define IRQ_FLOPPY          6       // Floppy Disk Controller
#define IRQ_LPT1            7       // Parallel Port #1
#define IRQ_RTC             8       // Real-Time Clock (RTC)
#define IRQ_ACPI            9       // ACPI Control Interrupt
#define IRQ_MISC1           10      // (possibly scsi or nic)
#define IRQ_MISC2           11      // (possibly scsi or nic)
#define IRQ_MOUSE           12      // PS/2 Mouse
#define IRQ_COPOCESSOR      13      // Coprocessor Interrupt
#define IRQ_ATA1            14      // ATA Channel #1
#define IRQ_ATA2            15      // ATA Channel #2
#define NUM_IRQ             16

/**
 * Intel exception vector numbers.
 */
#define EXCEPTION_DE        0x00    // Divide Error
#define EXCEPTION_DB        0x01    // Debug Exception
#define EXCEPTION_NMI       0x02    // Non-Maskable Interrupt
#define EXCEPTION_BP        0x03    // Breakpoint
#define EXCEPTION_OF        0x04    // Overflow
#define EXCEPTION_BR        0x05    // BOUND Range Exceeded
#define EXCEPTION_UD        0x06    // Invalid Opcode
#define EXCEPTION_NM        0x07    // Device Not Available
#define EXCEPTION_DF        0x08    // Double Fault
#define EXCEPTION_TS        0x0A    // Invalid TSS
#define EXCEPTION_NP        0x0B    // Segment Not Present
#define EXCEPTION_SS        0x0C    // Stack Fault
#define EXCEPTION_GP        0x0D    // General Protection Fault
#define EXCEPTION_PF        0x0E    // Page Fault
#define EXCEPTION_MF        0x10    // Math Fault (x87 FPU Floating-Point Error)
#define EXCEPTION_AC        0x11    // Alignment Check
#define EXCEPTION_MC        0x12    // Machine Check
#define EXCEPTION_XM        0x13    // SIMD Floating-Point Exception
#define EXCEPTION_VE        0x14    // Virtualization Exception
#define EXCEPTION_CP        0x15    // Control Protection Exception
#define NUM_EXCEPTION       32

/**
 * Other interrupt vector numbers.
 */
#define INT_EXCEPTION       0x00    // Base interrupt vector for exceptions.
#define INT_IRQ             0x20    // Base interurpt vector for device IRQs.
#define INT_SYSCALL         0x80    // Interrupt vector for syscalls.

/**
 * Interrupt register frame offsets.
 */
#define IREGS_EBX           0x00
#define IREGS_ECX           0x04
#define IREGS_EDX           0x08
#define IREGS_ESI           0x0C
#define IREGS_EDI           0x10
#define IREGS_EBP           0x14
#define IREGS_EAX           0x18
#define IREGS_DS            0x1C
#define IREGS_ES            0x1E
#define IREGS_FS            0x20
#define IREGS_GS            0x22
#define IREGS_VEC_NUM       0x24
#define IREGS_ERR_CODE      0x28
#define IREGS_EIP           0x2C
#define IREGS_CS            0x30
#define IREGS_EFLAGS        0x34
#define IREGS_ESP           0x38
#define IREGS_SS            0x3C

#define IREGS_MAIN_SIZE     (IREGS_VEC_NUM-IREGS_EBX)

#ifndef __ASSEMBLER__

/**
 * Current process's register state upon entry to the kernel via interrupt.
 */
struct iregs
{
    uint32_t ebx;       // pushed by main handler
    uint32_t ecx;       // pushed by main handler
    uint32_t edx;       // pushed by main handler
    uint32_t esi;       // pushed by main handler
    uint32_t edi;       // pushed by main handler
    uint32_t ebp;       // pushed by main handler
    uint32_t eax;       // pushed by main handler, not restored for syscall
    uint16_t ds;        // pushed by main handler
    uint16_t es;        // pushed by main handler
    uint16_t fs;        // pushed by main handler
    uint16_t gs;        // pushed by main handler

    uint32_t vec_num;   // pushed by thunk
    uint32_t err_code;  // pushed by cpu or thunk

    uint32_t eip;       // pushed by cpu
    uint32_t cs;        // pushed by cpu
    uint32_t eflags;    // pushed by cpu
    uint32_t esp;       // pushed by cpu, only present when privilege level changes
    uint32_t ss;        // pushed by cpu, only present when privilege level changes
} __align(4);

static_assert(offsetof(struct iregs, ebx) == IREGS_EBX, "offsetof(struct iregs, ebx) == IREGS_EBX");
static_assert(offsetof(struct iregs, ecx) == IREGS_ECX, "offsetof(struct iregs, ecx) == IREGS_ECX");
static_assert(offsetof(struct iregs, edx) == IREGS_EDX, "offsetof(struct iregs, edx) == IREGS_EDX");
static_assert(offsetof(struct iregs, esi) == IREGS_ESI, "offsetof(struct iregs, esi) == IREGS_ESI");
static_assert(offsetof(struct iregs, edi) == IREGS_EDI, "offsetof(struct iregs, edi) == IREGS_EDI");
static_assert(offsetof(struct iregs, ebp) == IREGS_EBP, "offsetof(struct iregs, ebp) == IREGS_EBP");
static_assert(offsetof(struct iregs, eax) == IREGS_EAX, "offsetof(struct iregs, eax) == IREGS_EAX");
static_assert(offsetof(struct iregs, ds) == IREGS_DS, "offsetof(struct iregs, ds) == IREGS_DS");
static_assert(offsetof(struct iregs, es) == IREGS_ES, "offsetof(struct iregs, es) == IREGS_ES");
static_assert(offsetof(struct iregs, fs) == IREGS_FS, "offsetof(struct iregs, fs) == IREGS_FS");
static_assert(offsetof(struct iregs, gs) == IREGS_GS, "offsetof(struct iregs, gs) == IREGS_GS");
static_assert(offsetof(struct iregs, vec_num) == IREGS_VEC_NUM, "offsetof(struct iregs, vec_num) == IREGS_VEC_NUM");
static_assert(offsetof(struct iregs, err_code) == IREGS_ERR_CODE, "offsetof(struct iregs, err_code) == IREGS_ERR_CODE");
static_assert(offsetof(struct iregs, eip) == IREGS_EIP, "offsetof(struct iregs, eip) == IREGS_EIP");
static_assert(offsetof(struct iregs, cs) == IREGS_CS, "offsetof(struct iregs, cs) == IREGS_CS");
static_assert(offsetof(struct iregs, eflags) == IREGS_EFLAGS, "offsetof(struct iregs, eflags) == IREGS_EFLAGS");
static_assert(offsetof(struct iregs, esp) == IREGS_ESP, "offsetof(struct iregs, esp) == IREGS_ESP");
static_assert(offsetof(struct iregs, ss) == IREGS_SS, "offsetof(struct iregs, ss) == IREGS_SS");

__fastcall void recv_interrupt(struct iregs *regs);
__fastcall void recv_irq(struct iregs *regs);
__fastcall void recv_syscall(struct iregs *regs);

typedef void (__fastcall *idt_thunk)(void);
/*idt_thunk*/ __fastcall void _thunk_except00h(void);
/*idt_thunk*/ __fastcall void _thunk_except01h(void);
/*idt_thunk*/ __fastcall void _thunk_except02h(void);
/*idt_thunk*/ __fastcall void _thunk_except03h(void);
/*idt_thunk*/ __fastcall void _thunk_except04h(void);
/*idt_thunk*/ __fastcall void _thunk_except05h(void);
/*idt_thunk*/ __fastcall void _thunk_except06h(void);
/*idt_thunk*/ __fastcall void _thunk_except07h(void);
/*idt_thunk*/ __fastcall void _thunk_except08h(void);
/*idt_thunk*/ __fastcall void _thunk_except09h(void);
/*idt_thunk*/ __fastcall void _thunk_except0Ah(void);
/*idt_thunk*/ __fastcall void _thunk_except0Bh(void);
/*idt_thunk*/ __fastcall void _thunk_except0Ch(void);
/*idt_thunk*/ __fastcall void _thunk_except0Dh(void);
/*idt_thunk*/ __fastcall void _thunk_except0Eh(void);
/*idt_thunk*/ __fastcall void _thunk_except0Fh(void);
/*idt_thunk*/ __fastcall void _thunk_except10h(void);
/*idt_thunk*/ __fastcall void _thunk_except11h(void);
/*idt_thunk*/ __fastcall void _thunk_except12h(void);
/*idt_thunk*/ __fastcall void _thunk_except13h(void);
/*idt_thunk*/ __fastcall void _thunk_except14h(void);
/*idt_thunk*/ __fastcall void _thunk_except15h(void);
/*idt_thunk*/ __fastcall void _thunk_except16h(void);
/*idt_thunk*/ __fastcall void _thunk_except17h(void);
/*idt_thunk*/ __fastcall void _thunk_except18h(void);
/*idt_thunk*/ __fastcall void _thunk_except19h(void);
/*idt_thunk*/ __fastcall void _thunk_except1Ah(void);
/*idt_thunk*/ __fastcall void _thunk_except1Bh(void);
/*idt_thunk*/ __fastcall void _thunk_except1Ch(void);
/*idt_thunk*/ __fastcall void _thunk_except1Dh(void);
/*idt_thunk*/ __fastcall void _thunk_except1Eh(void);
/*idt_thunk*/ __fastcall void _thunk_except1Fh(void);
/*idt_thunk*/ __fastcall void _thunk_irq00h(void);
/*idt_thunk*/ __fastcall void _thunk_irq01h(void);
/*idt_thunk*/ __fastcall void _thunk_irq02h(void);
/*idt_thunk*/ __fastcall void _thunk_irq03h(void);
/*idt_thunk*/ __fastcall void _thunk_irq04h(void);
/*idt_thunk*/ __fastcall void _thunk_irq05h(void);
/*idt_thunk*/ __fastcall void _thunk_irq06h(void);
/*idt_thunk*/ __fastcall void _thunk_irq07h(void);
/*idt_thunk*/ __fastcall void _thunk_irq08h(void);
/*idt_thunk*/ __fastcall void _thunk_irq09h(void);
/*idt_thunk*/ __fastcall void _thunk_irq0Ah(void);
/*idt_thunk*/ __fastcall void _thunk_irq0Bh(void);
/*idt_thunk*/ __fastcall void _thunk_irq0Ch(void);
/*idt_thunk*/ __fastcall void _thunk_irq0Dh(void);
/*idt_thunk*/ __fastcall void _thunk_irq0Eh(void);
/*idt_thunk*/ __fastcall void _thunk_irq0Fh(void);
/*idt_thunk*/ __fastcall void _thunk_syscall(void);

#endif /* __ASSEMBLER__ */

#endif /* __INTERRUPT_H */
