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
 *         File: include/interrupt.h
 *      Created: January 6, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __INTERRUPT_H
#define __INTERRUPT_H

/**
 * Interrupt vector table regions.
 */
#define EXCEPTION_BASE_VECTOR       0x00
#define IRQ_BASE_VECTOR             0x20
#define SYSCALL_VECTOR              0x80
#define NR_INTERRUPT_VECTORS        256

/**
 * Intel exception vector numbers.
 */
#define DIVIDE_ERROR                0x00
#define DEBUG_EXCEPTION             0x01
#define NMI_INTERRUPT               0x02
#define BREAKPOINT                  0x03
#define OVERFLOW_EXCEPTION          0x04
#define BOUND_RANGE_EXCEEDED        0x05
#define INVALID_OPCODE              0x06
#define DEVICE_NOT_AVAILABLE        0x07
#define DOUBLE_FAULT                0x08
#define SEGMENT_OVERRUN             0x09
#define INVALID_TSS                 0x0A
#define SEGMENT_NOT_PRESENT         0x0B
#define STACK_FAULT                 0x0C
#define PROTECTION_FAULT            0x0D
#define PAGE_FAULT                  0x0E
#define MATH_FAULT                  0x10
#define ALIGNMENT_CHECK             0x11
#define MACHINE_CHECK               0x12
#define SIMD_FAULT                  0x13
#define NR_EXCEPTIONS               32

/**
 * Interrupt register frame offsets.
 */
#define IREGS_EBX                   0x00
#define IREGS_ECX                   0x04
#define IREGS_EDX                   0x08
#define IREGS_ESI                   0x0C
#define IREGS_EDI                   0x10
#define IREGS_EBP                   0x14
#define IREGS_EAX                   0x18
#define IREGS_DS                    0x1C
#define IREGS_ES                    0x20
#define IREGS_FS                    0x24
#define IREGS_GS                    0x28
#define IREGS_VEC                   0x2C
#define IREGS_ERR                   0x30
#define IREGS_EIP                   0x34
#define IREGS_CS                    0x38
#define IREGS_EFLAGS                0x3C
#define IREGS_ESP                   0x40
#define IREGS_SS                    0x44
#define SIZEOF_IREGS                72

/**
 * Interrupt stack offsets after executing thunk routine.
 */
#define IRET_VEC                    0x00
#define IRET_ERR                    0x04
#define IRET_EIP                    0x08    // technically, iret regs start here
#define IRET_CS                     0x0C
#define IRET_EFLAGS                 0x10
#define IRET_ESP                    0x14
#define IRET_SS                     0x18

#ifndef __ASSEMBLER__
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (__fastcall *idt_thunk)(void);

struct iregs {
    // program context regs
    uint32_t ebx;   // syscall param0
    uint32_t ecx;   // syscall param1
    uint32_t edx;   // syscall param2
    uint32_t esi;   // syscall param3
    uint32_t edi;   // syscall param4
    uint32_t ebp;   // syscall param5
    uint32_t eax;   // syscall return value
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;

// interrupt info
    int32_t vec;    // interrupt vector number (negative for IRQ)
    uint32_t err;   // exception error code

// iret regs
    uint32_t eip;
    uint32_t cs;    // cpu pushes 32 bits
    uint32_t eflags;
    uint32_t esp;   // only present if user process interrupted
    uint32_t ss;    // only present if user process interrupted
};
static_assert(offsetof(struct iregs, ebx) == IREGS_EBX, "offsetof(struct iregs, ebx)");
static_assert(offsetof(struct iregs, ecx) == IREGS_ECX, "offsetof(struct iregs, ecx)");
static_assert(offsetof(struct iregs, edx) == IREGS_EDX, "offsetof(struct iregs, edx)");
static_assert(offsetof(struct iregs, esi) == IREGS_ESI, "offsetof(struct iregs, esi)");
static_assert(offsetof(struct iregs, edi) == IREGS_EDI, "offsetof(struct iregs, edi)");
static_assert(offsetof(struct iregs, ebp) == IREGS_EBP, "offsetof(struct iregs, ebp)");
static_assert(offsetof(struct iregs, eax) == IREGS_EAX, "offsetof(struct iregs, eax)");
static_assert(offsetof(struct iregs, ds) == IREGS_DS, "offsetof(struct iregs, ds)");
static_assert(offsetof(struct iregs, es) == IREGS_ES, "offsetof(struct iregs, es)");
static_assert(offsetof(struct iregs, fs) == IREGS_FS, "offsetof(struct iregs, fs)");
static_assert(offsetof(struct iregs, gs) == IREGS_GS, "offsetof(struct iregs, gs)");
static_assert(offsetof(struct iregs, vec) == IREGS_VEC, "offsetof(struct iregs, vec)");
static_assert(offsetof(struct iregs, err) == IREGS_ERR, "offsetof(struct iregs, err)");
static_assert(offsetof(struct iregs, eip) == IREGS_EIP, "offsetof(struct iregs, eip)");
static_assert(offsetof(struct iregs, cs) == IREGS_CS, "offsetof(struct iregs, cs)");
static_assert(offsetof(struct iregs, eflags) == IREGS_EFLAGS, "offsetof(struct iregs, eflags)");
static_assert(offsetof(struct iregs, esp) == IREGS_ESP, "offsetof(struct iregs, esp)");
static_assert(offsetof(struct iregs, ss) == IREGS_SS, "offsetof(struct iregs, ss)");
static_assert(sizeof(struct iregs) == SIZEOF_IREGS, "sizeof(struct iregs)");

__fastcall void switch_context(struct iregs *regs);     // see entry.S


/**
 * Saves the EFLAGS register, then clears interrupts.
 *
 * @param flags - a 32-bit number used for storing the EFLAGS register;
 *                note this NOT a pointer, due to the way GCC inline assembly
 *                handles parameters
 */
#define cli_save(flags)                                                     \
__asm__ volatile (                                                          \
    "                                                                       \n\
    pushfl                                                                  \n\
    popl %0                                                                 \n\
    cli                                                                     \n\
    "                                                                       \
    : "=r"(flags)                                                           \
)

/**
 * Sets the EFLAGS register.
 *
 * @param flags - a 32-bit number containing the EFLAGS to be set
 */
#define restore_flags(flags)                                                \
__asm__ volatile (                                                          \
    "                                                                       \n\
    push %0                                                                 \n\
    popfl                                                                   \n\
    "                                                                       \
    :                                                                       \
    : "r"(flags)                                                            \
)

#endif /* __ASSEMBLER__ */

#endif /* __INTERRUPT_H */
