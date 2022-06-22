/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
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

/* Page Directory Entry Bits */
#define PDE_P       (1 << 0)        /* Present */
#define PDE_RW      (1 << 1)        /* Read/Write */
#define PDE_US      (1 << 2)        /* User/Supervisor */
#define PDE_PWT     (1 << 3)        /* Page-level Write Through */
#define PDE_PCD     (1 << 4)        /* Page-level Cache Disable */
#define PDE_A       (1 << 5)        /* Accessed */
#define PDE_D       (1 << 6)        /* Dirty */
#define PDE_PS      (1 << 7)        /* Page Size */
#define PDE_G       (1 << 8)        /* Page Global */
#define PDE_PTB     (1 << 12)       /* Page Table Base Address (when PS = 0) */
#define PDE_PFB     (1 << 22)       /* Page Frame Base Address (when PS = 1) */

/* Page Table Entry Bits */
#define PTE_P       (1 << 0)        /* Present */
#define PTE_RW      (1 << 1)        /* Read/Write */
#define PTE_US      (1 << 2)        /* User/Supervisor */
#define PTE_PWT     (1 << 3)        /* Page-level Write Through */
#define PTE_PCD     (1 << 4)        /* Page-level Cache Disable */
#define PTE_A       (1 << 5)        /* Accessed */
#define PTE_D       (1 << 6)        /* Dirty */
#define PTE_G       (1 << 8)        /* Page Global */
#define PTE_PFB     (1 << 12)       /* Page Frame Base Address */

#ifndef __ASSEMBLER__
#include <stdint.h>

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
            uint32_t base   : 10;   /* 4 MiB Page Frame Base Address */
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
            uint32_t base   : 20;   /* 4 KiB Page Frame Base Address */
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

#endif /* __ASSEMBLER__ */

#endif /* __X86_PAGING_H */
