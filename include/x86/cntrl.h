/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: include/x86/cntrl.h                                               *
 * Created: December 20, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * x86 Control Registers.
 * See Intel IA-32 Software Developer's Manual, Volume 3 for more information *
 * on each structure.                                                         *
 *============================================================================*/

#ifndef __X86_CTRNL_H
#define __X86_CTRNL_H

#define CR0_PE      (1 << 0)        /* Protected Mode Enable */
#define CR0_PG      (1 << 31)       /* Paging Enable */
#define CR3_PWT     (1 << 3)        /* Page-level Write-Through (Page Directory) */
#define CR3_PCD     (1 << 4)        /* Page-level Cache Disable (Page Directory) */
#define CR3_PDB     (1 << 12)       /* Page Directory Base Address */
#define CR4_PSE     (1 << 4)        /* Page Size Enable */
#define CR4_PAE     (1 << 5)        /* Physical Address Extension Enable */
#define CR4_PGE     (1 << 7)        /* Page Global-bit Enable */

#ifndef __ASSEMBLY__
#include <stdint.h>

/**
 * Control Register 0
 * Contains system control flags that control operating mode and states of the
 * processor.
 */
struct cr0 {
    union {
        struct {
            uint32_t pe : 1;    /* Protection Enable */
            uint32_t mp : 1;    /* Monitor Coprocessor */
            uint32_t em : 1;    /* x87 Emulation */
            uint32_t ts : 1;    /* Task Switched */
            uint32_t et : 1;    /* Extension Type */
            uint32_t ne : 1;    /* Numeric Error */
            uint32_t    : 10;   /* (reserved) */
            uint32_t wp : 1;    /* Write Protect */
            uint32_t    : 1;    /* (reserved) */
            uint32_t am : 1;    /* Alignment Mask */
            uint32_t    : 10;   /* (reserved) */
            uint32_t nw : 1;    /* Not Write-Through */
            uint32_t cd : 1;    /* Cache Disable */
            uint32_t pg : 1;    /* Paging */
        };
        uint32_t _value;
    };
};
_Static_assert(sizeof(struct cr0) == 4, "sizeof(struct cr0)");

/**
 * Control Register 3
 * Contains the physical address of the base of the paging-structure hierarchy
 * and two paging flags.
 */
struct cr3 {
    union {
        struct {
            uint32_t            : 3;    /* (reserved) */
            uint32_t pwt        : 1;    /* Page-level Write-Through */
            uint32_t pcd        : 1;    /* Page-level Cache Disable */
            uint32_t            : 7;    /* (reserved) */
            uint32_t pgdir_base : 20;   /* Page Directory Base Address */
        };
        uint32_t _value;
    };
};
_Static_assert(sizeof(struct cr3) == 4, "sizeof(struct cr3)");

/**
 * Control Register 4
 * Contains a group of flags that enable several architectural extensions, and
 * indicate operating system or executive support for specific processor
 * capabilities.
 */
struct cr4 {
    union {
        struct {
            uint32_t vme        : 1;    /* Virtual 8086 Mode Extensions */
            uint32_t pvi        : 1;    /* Protected Mode Virtual Interrupts */
            uint32_t tsd        : 1;    /* Time Stamp Disable */
            uint32_t de         : 1;    /* Debugging Extensions */
            uint32_t pse        : 1;    /* Page Size Extensions */
            uint32_t pae        : 1;    /* Physical Address Extension */
            uint32_t mce        : 1;    /* Machine Check Enable */
            uint32_t pge        : 1;    /* Page Global-bit Enable */
            uint32_t pce        : 1;    /* Performance Monitoring Counter Enable */
            uint32_t osfxsr     : 1;    /* FXSAVE & FXRSTOR Support */
            uint32_t osmmxexcpt : 1;    /* Unmasked SIMD Floating-Point Exception Support*/
            uint32_t umip       : 1;    /* User Mode Instruction Prevention */
            uint32_t la57       : 1;    /* 57-bit Linear Addresses */
            uint32_t vmxe       : 1;    /* VMX Enable */
            uint32_t smxe       : 1;    /* SMX Enable */
            uint32_t            : 1;    /* (reserved) */
            uint32_t fsgsbase   : 1;    /* FSGSBASE Enable */
            uint32_t pcide      : 1;    /* PCID Enable */
            uint32_t osxsave    : 1;    /* XSAVE and Processor Extended States Enable */
            uint32_t            : 1;    /* (reserved) */
            uint32_t smep       : 1;    /* SMEP Enable */
            uint32_t smap       : 1;    /* SMAP Enable */
            uint32_t pke        : 1;    /* Enable Protection Keys for User Pages */
            uint32_t cet        : 1;    /* Control-flow Enforcement Technology */
            uint32_t pks        : 1;    /* Enable Protection Keys for Supervisor Pages */
            uint32_t            : 7;    /* (reserved) */
        };
        uint32_t _value;
    };
};
_Static_assert(sizeof(struct cr4) == 4, "sizeof(struct cr4)");

/**
 * Reads the CR0 register.
 */
#define rdcr0(cr0)          \
__asm__ volatile (          \
    "movl %%cr0, %%eax"     \
    : "=a"(cr0)             \
    :                       \
    :                       \
)

/**
 * Writes the CR0 register.
 */
#define wrcr0(cr0)          \
__asm__ volatile (          \
    "movl %%eax, %%cr0"     \
    :                       \
    : "a"(cr0)              \
    :                       \
)

/**
 * Reads the CR2 register.
 */
#define rdcr2(cr2)          \
__asm__ volatile (          \
    "movl %%cr2, %%eax"     \
    : "=a"(cr2)             \
    :                       \
    :                       \
)

/**
 * Writes the CR2 register.
 */
#define wrcr2(cr2)          \
__asm__ volatile (          \
    "movl %%eax, %%cr2"     \
    :                       \
    : "a"(cr2)              \
    :                       \
)

/**
 * Reads the CR3 register.
 */
#define rdcr3(cr0)          \
__asm__ volatile (          \
    "movl %%cr3, %%eax"     \
    : "=a"(cr3)             \
    :                       \
    :                       \
)

/**
 * Writes the CR3 register.
 */
#define wrcr3(cr3)          \
__asm__ volatile (          \
    "movl %%eax, %%cr3"     \
    :                       \
    : "a"(cr3)              \
    :                       \
)

/**
 * Reads the CR4 register.
 */
#define rdcr4(cr0)          \
__asm__ volatile (          \
    "movl %%cr4, %%eax"     \
    : "=a"(cr4)             \
    :                       \
    :                       \
)

/**
 * Writes the CR4 register.
 */
#define wrcr4(cr4)          \
__asm__ volatile (          \
    "movl %%eax, %%cr4"     \
    :                       \
    : "a"(cr4)              \
    :                       \
)

#endif /* __ASSEMBLY__ */

#endif /* __X86_CTRNL_H */
