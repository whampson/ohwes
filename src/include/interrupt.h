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
 * Important interrupt vector table numbers.
 */
#define IVT_EXCEPTION       0x00    // Base interrupt vector for exceptions.
#define IVT_DEVICEIRQ       0x20    // Base interurpt vector for device IRQs.
#define IVT_SYSCALL         0x80    // Interrupt vector for syscalls.

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

#define SIZEOF_IREGS_CTX    (IREGS_VEC_NUM-IREGS_EBX)
#define SIZEOF_IREGS        0x40

#ifndef __ASSEMBLER__

/**
 * Register state upon receiving an interrupt.
 */
struct iregs
{
// program context regs
    uint32_t ebx;       // pushed by common handler
    uint32_t ecx;       // pushed by common handler
    uint32_t edx;       // pushed by common handler
    uint32_t esi;       // pushed by common handler
    uint32_t edi;       // pushed by common handler
    uint32_t ebp;       // pushed by common handler
    uint32_t eax;       // pushed by common handler, not restored for syscall
    uint16_t ds;        // pushed by common handler
    uint16_t es;        // pushed by common handler
    uint16_t fs;        // pushed by common handler
    uint16_t gs;        // pushed by common handler

// interrupt info
    uint32_t vec_num;   // pushed by thunk
    uint32_t err_code;  // pushed by cpu or thunk

// cpu control regs (system context, iret regs)
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
static_assert(sizeof(struct iregs) == SIZEOF_IREGS, "sizeof(struct iregs) == SIZEOF_IREGS");

__fastcall void recv_interrupt(struct iregs *regs);
__fastcall void recv_irq(struct iregs *regs);
__fastcall int recv_syscall(struct iregs *regs);

__fastcall void switch_context(struct iregs *regs);

typedef void (__fastcall *idt_thunk)(void);

#endif /* __ASSEMBLER__ */

#endif /* __INTERRUPT_H */
