/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
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
 *    File: include/x86/paging.h                                              *
 * Created: December 20, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * 32-bit Paging Structure definitions for x86-family CPUs.                   *
 * See Intel IA-32 Software Developer's Manual, Volume 3 for more information *
 * on each structure.                                                         *
 *============================================================================*/

#ifndef __X86_PAGING_H
#define __X86_PAGING_H

#include <stdint.h>

#define PG_BIT      (1 << 31)   /* CR0 - enable paging */
#define PSE_BIT     (1 << 4)    /* CR4 - allow for 4 MiB pages */

struct pgdir_entry
{
    union {
        struct pde4k {              /* Entry maps to a 4 KiB page table. */
            uint32_t p      : 1;    /* Present; set to 1 if in-use */
            uint32_t rw     : 1;    /* Read/Write; 0 = read only, 1 = r/w */
            uint32_t us     : 1;    /* User/Supervisor; 0 = super, 1 = user */
            uint32_t pwt    : 1;    /* Cache Policy; 0 = write-back, 1 = write-through */
            uint32_t pcd    : 1;    /* Page Cache Disable; 0 = enabled, 1 = disabled */
            uint32_t a      : 1;    /* Accessed */
            uint32_t        : 1;    /* (ignored) */
            uint32_t ps     : 1;    /* Page Size; must be 0 for 4K page table */
            uint32_t        : 4;    /* (ignored) */
            uint32_t base   : 20;   /* Page Table Base Address */
        } pde4k;
        struct pde4m {              /* Entry maps to a 4 MiB page. */
            uint32_t p      : 1;    /* Present; set to 1 if in-use */
            uint32_t rw     : 1;    /* Read/Write; 0 = read only, 1 = r/w */
            uint32_t us     : 1;    /* User/Supervisor; 0 = super, 1 = user */
            uint32_t pwt    : 1;    /* Cache Policy; 0 = write-back, 1 = write-through */
            uint32_t pcd    : 1;    /* Page Cache Disable; 0 = enabled, 1 = disabled */
            uint32_t a      : 1;    /* Accessed */
            uint32_t d      : 1;    /* Dirty */
            uint32_t ps     : 1;    /* Using Page Attribute Table */
            uint32_t g      : 1;    /* Global */
            uint32_t        : 3;    /* (ignored) */
            uint32_t pat    : 1;    /* Physical Address Translation */
            uint32_t        : 9;    /* reserved, set to 0 (PAE bits) */
            uint32_t base   : 10;   /* 4 MiB page base */
        } pde4m;
        uint32_t _value;
    };
};
_Static_assert(sizeof(struct pgdir_entry) == 4, "sizeof(struct pgdir_entry)");

struct pgtbl_entry
{
    union {
        struct {
            uint32_t p      : 1;    /* Present; set to 1 if in-use */
            uint32_t rw     : 1;    /* Read/Write; 0 = read only, 1 = r/w */
            uint32_t us     : 1;    /* User/Supervisor; 0 = super, 1 = user */
            uint32_t pwt    : 1;    /* Cache Policy; 0 = write-back, 1 = write-through */
            uint32_t pcd    : 1;    /* Page Cache Disable; 0 = enabled, 1 = disabled */
            uint32_t a      : 1;    /* Accessed */
            uint32_t d      : 1;    /* Dirty */
            uint32_t pat    : 1;    /* Physical Address Translation */
            uint32_t g      : 1;    /* Global */
            uint32_t        : 3;    /* (ignored) */
            uint32_t base   : 20;   /* 4 KiB page base */
        };
        uint32_t _value;
    };
};
_Static_assert(sizeof(struct pgtbl_entry) == 4, "sizeof(struct pgtbl_entry)");

#define flush_tlb()                 \
__asm__ volatile (                  \
    "                               \n\
    movl    %cr3, %eax              \n\
    movl    %eax, %cr3              \n\
    "                               \
)

#endif /* __X86_PAGING_H */
