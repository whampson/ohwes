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
 *    File: include/nb/x86_desc.h                                             *
 * Created: December 12, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Structure and descriptor definitions for x86-family CPUs.                  *
 * See Intel IA-32 Software Developer's Manual, Volume 3 for more information *
 * on each structure.                                                         *
 *============================================================================*/

#ifndef __X86_DESC_H
#define __X86_DESC_H

#include <string.h>

/* System Segment Descriptor Types */
#define SEGDESC_TYPE_TSS16      0x01    /* System; Task State Segment (16-bit) */
#define SEGDESC_TYPE_LDT        0x02    /* System; Local Descriptor Table */
#define SEGDESC_TYPE_CALL16     0x04    /* System; Call Gate (16-bit) */
#define SEGDESC_TYPE_TASK       0x05    /* System; Task Gate */
#define SEGDESC_TYPE_INT16      0x06    /* System; Interrupt Gate (16-bit) */
#define SEGDESC_TYPE_TRAP16     0x07    /* System; Trap Gate (16-bit) */
#define SEGDESC_TYPE_TSS32      0x09    /* System; Task State Segment (32-bit) */
#define SEGDESC_TYPE_CALL32     0x0C    /* System; Call Gate (32-bit) */
#define SEGDESC_TYPE_INT32      0x0E    /* System; Interrupt Gate (32-bit) */
#define SEGDESC_TYPE_TRAP32     0x0F    /* System; Trap Gate (32-bit) */

/* Code- and Data-Segment Descriptor Types */
#define SEGDESC_TYPE_R          0x00    /* Data; Read-Only */
#define SEGDESC_TYPE_RA         0x01    /* Data; Read-Only, Accessed */
#define SEGDESC_TYPE_RW         0x02    /* Data; Read/Write */
#define SEGDESC_TYPE_RWA        0x03    /* Data; Read/Write, Accessed */
#define SEGDESC_TYPE_RE         0x04    /* Data; Read-Only, Expand-Down */
#define SEGDESC_TYPE_REA        0x05    /* Data; Read-Only, Expand-Down, Accessed */
#define SEGDESC_TYPE_RWE        0x06    /* Data; Read/Write, Expand-Down */
#define SEGDESC_TYPE_RWEA       0x07    /* Data; Read/Write, Expand-Down, Accessed */
#define SEGDESC_TYPE_X          0x08    /* Code; Execute-Only */
#define SEGDESC_TYPE_XA         0x09    /* Code; Execute-Only, Accessed */
#define SEGDESC_TYPE_XR         0x0A    /* Code; Execute/Read */
#define SEGDESC_TYPE_XRA        0x0B    /* Code; Execute/Read, Accessed */
#define SEGDESC_TYPE_XC         0x0C    /* Code; Execute-Only, Conforming */
#define SEGDESC_TYPE_XCA        0x0D    /* Code; Execute-Only, Conforming, Accessed */
#define SEGDESC_TYPE_XRC        0x0E    /* Code; Execute/Read, Conforming */
#define SEGDESC_TYPE_XRCA       0x0F    /* Code; Execute/Read, Conforming */

#ifndef __ASSEMBLY__
#include <stdint.h>

/**
 * Segment Selector
 */
typedef union
{
    struct
    {
        uint16_t rpl    : 2;        /* requested privilege level */
        uint16_t ti     : 1;        /* table type; 0 = GDT, 1 = LDT */
        uint16_t index  : 13;       /* descriptor table index */
    };
    uint16_t _value;
} segsel_t;

_Static_assert(sizeof(segsel_t) == 2, "Invalid Segment Selector size!");

/**
 * Segment Descriptor
 */
typedef union
{
    struct
    {
        uint64_t limit_lo   : 16;   /* Segment Limit (bits 15:0) */
        uint64_t base_lo    : 24;   /* Segment Base (bits 23:0) */
        uint64_t type       : 4;    /* Segment/Gate Type */
        uint64_t s          : 1;    /* Descriptor Type; 0 = System, 1 = Code/Data */
        uint64_t dpl        : 2;    /* Descriptor Privilege Level */
        uint64_t p          : 1;    /* Present Bit */
        uint64_t limit_hi   : 4;    /* Segment Limit (bits 20:16) */
        uint64_t avl        : 1;    /* Available for Use */
        uint64_t            : 1;    /* Reserved; Do Not Use */
        uint64_t db         : 1;    /* 0 = 16 bit Code/Data, 1 = 32-bit Code/Data */
        uint64_t g          : 1;    /* Granularity; 0 = Byte, 1 = 4K Page */
        uint64_t base_hi    : 8;    /* Segment Base (bits 31:24) */
    } gdt_ldt;
    struct
    {
        uint64_t limit_lo   : 16;
        uint64_t base_lo    : 24;
        uint64_t type       : 4;
        uint64_t            : 1;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t limit_hi   : 4;
        uint64_t avl        : 1;
        uint64_t            : 1;
        uint64_t            : 1;
        uint64_t g          : 1;
        uint64_t base_hi    : 8;
    } tss;
    struct
    {
        uint64_t offset_lo  : 16;
        uint64_t segsel     : 16;
        uint64_t param_count: 5;
        uint64_t            : 3;
        uint64_t type       : 4;
        uint64_t            : 1;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t offset_hi  : 16;
    } call_gate;
    struct
    {
        uint64_t            : 16;
        uint64_t tss_segsel : 16;
        uint64_t            : 8;
        uint64_t type       : 4;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t            : 16;
    } task_gate;
    struct
    {
        uint64_t offset_lo  : 16;
        uint64_t segsel     : 16;
        uint64_t            : 8;
        uint64_t type       : 4;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t offset_hi  : 16;
    } int_gate;
    struct
    {
        uint64_t offset_lo  : 16;
        uint64_t segsel     : 16;
        uint64_t            : 8;
        uint64_t type       : 4;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t offset_hi  : 16;
    } trap_gate;
    uint64_t _value;
} segdesc_t;

_Static_assert(sizeof(segdesc_t) == 8, "Invalid Segment Descriptor size!");

/**
 * Descriptor Register (for LGDT and LIDT instructions).
 */
typedef union
{
    struct
    {
        uint64_t limit  : 16;
        uint64_t base   : 32;
        uint64_t        : 16;
    };
    uint64_t _value;
} descreg_t;

_Static_assert(sizeof(descreg_t) == 8, "Invalid Descriptor Register size");

/**
 * Task State Segment
 */
struct tss
{
    uint16_t prev_task;
    uint16_t _reserved0;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t _reserved1;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t _reserved2;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t _reserved3;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t _reserved4;
    uint16_t cs;
    uint16_t _reserved5;
    uint16_t ss;
    uint16_t _reserved6;
    uint16_t ds;
    uint16_t _reserved7;
    uint16_t fs;
    uint16_t _reserved8;
    uint16_t gs;
    uint16_t _reserved9;
    uint16_t ldt_segsel;
    uint16_t _reserved10;
    uint16_t debug_trap : 1;
    uint16_t _reserved11 : 15;
    uint16_t io_map_base;
    uint32_t ssp;
};

_Static_assert(sizeof(struct tss) == 108, "Invalid TSS size!");

/**
 * Gets a segment descriptor from a descriptor table.
 * 
 * @param table the descriptor table
 * @param selector the segment selector
 */
#define get_segdesc(table,selector) (table + (selector / sizeof(segdesc_t)))

/**
 * Sets the values in a GDT or LDT code/data segment descriptor.
 * 
 * @param table a pointer to the descriptor table
 * @param selector a segment selector (offset in table)
 * @param base the segment base address
 * @param limit the segment limit (segment size in 4K pages - 1)
 * @param desc_type the segment descriptor type (one of SEGDESC_TYPE_*)
 * @param desc_priv the segment descriptor privilege level
 */
#define set_segdesc(table,selector,base,limit,desc_type,desc_priv)  \
do {                                                                \
    segdesc_t *desc = get_segdesc(table, selector);                 \
    desc->_value = 0;                                               \
    desc->gdt_ldt.base_lo = ((base) & 0x00FFFFFF);                  \
    desc->gdt_ldt.base_hi = ((base) & 0xFF000000) >> 24;            \
    desc->gdt_ldt.limit_lo = ((limit) & 0x0FFFF);                   \
    desc->gdt_ldt.limit_hi = ((limit) & 0xF0000) >> 16;             \
    desc->gdt_ldt.type = desc_type;                                 \
    desc->gdt_ldt.dpl = desc_priv;                                  \
    desc->gdt_ldt.g = 1;    /* 4K page granularity */               \
    desc->gdt_ldt.db = 1;   /* 32-bit segment */                    \
    desc->gdt_ldt.avl = 0;  /* not available */                     \
    desc->gdt_ldt.p = 1;    /* present */                           \
    desc->gdt_ldt.s = 1;    /* code/data segment */                 \
} while (0)

/**
 * Sets the values in a GDT or LDT system segment descriptor.
 * 
 * @param table a pointer to the descriptor table
 * @param selector a segment selector  (offset in table)
 * @param base the segment base address
 * @param limit the segment limit (segment size in 4K pages - 1)
 * @param desc_type the segment descriptor type (one of SEGDESC_TYPE_*)
 */
#define set_segdesc_sys(table,selector,base,limit,desc_type)        \
do {                                                                \
    segdesc_t *desc = get_segdesc(table, selector);                 \
    desc->_value = 0;                                               \
    desc->gdt_ldt.base_lo = ((base) & 0x00FFFFFF);                  \
    desc->gdt_ldt.base_hi = ((base) & 0xFF000000) >> 24;            \
    desc->gdt_ldt.limit_lo = ((limit) & 0x0FFFF);                   \
    desc->gdt_ldt.limit_hi = ((limit) & 0xF0000) >> 16;             \
    desc->gdt_ldt.type = desc_type;                                 \
    desc->gdt_ldt.dpl = 0;  /* ring 0 */                            \
    desc->gdt_ldt.g = 0;    /* byte granularity */                  \
    desc->gdt_ldt.db = 1;   /* 32-bit segment */                    \
    desc->gdt_ldt.avl = 0;  /* not available */                     \
    desc->gdt_ldt.p = 1;    /* present */                           \
    desc->gdt_ldt.s = 0;    /* system segment */                    \
} while (0)

/**
 * Sets the values in a segment descriptor for a TSS.
 * 
 * @param table a pointer to the descriptor table
 * @param selector a segment selector (offset in table)
 * @param base the TSS base address
 * @param limit the TSS limit (TSS size in bytes - 1)
 */
#define set_segdesc_tss(table,selector,base,limit)                  \
do {                                                                \
    segdesc_t *desc = get_segdesc(table, selector);                 \
    desc->_value = 0;                                               \
    desc->tss.base_lo = ((base) & 0x00FFFFFF);                      \
    desc->tss.base_hi = ((base) & 0xFF000000) >> 24;                \
    desc->tss.limit_lo = ((limit) & 0x0FFFF);                       \
    desc->tss.limit_hi = ((limit) & 0xF0000) >> 16;                 \
    desc->tss.type = SEGDESC_TYPE_TSS32;                            \
    desc->tss.dpl = 0;  /* ring 0 */                                \
    desc->tss.g = 0;    /* byte granularity */                      \
    desc->tss.avl = 0;  /* not available */                         \
    desc->tss.p = 1;    /* present */                               \
} while (0)

/**
 * Loads the Global Descriptor Table Register.
 *
 * @param descreg a descreg_t containing the GDT base and limit
 */
#define lgdt(descreg)       \
__asm__ volatile (          \
    "lgdt %0"               \
    :                       \
    : "m"(descreg)          \
    : "memory", "cc"        \
)

/**
 * Loads the Interrupt Descriptor Table Register.
 *
 * @param descreg a descreg_t containing the IDT base and limit
 */
#define lidt(descreg)       \
__asm__ volatile (          \
    "lidt %0"               \
    :                       \
    : "m"(descreg)          \
    : "memory", "cc"        \
)

/**
 * Loads the Local Descriptor Table Register.
 *
 * @param selector segment selector for the LDT
 */
#define lldt(selector)      \
__asm__ volatile (          \
    "lldt %w0"              \
    :                       \
    : "r"(selector)         \
    : "memory", "cc"        \
)

/**
 * Loads the Task Register.
 *
 * @param selector segment selector for the TSS
 */
#define ltr(selector)       \
__asm__ volatile (          \
    "ltr %w0"               \
    :                       \
    : "r"(selector)         \
    : "memory", "cc"        \
)

#define load_cs(cs) __asm__ volatile ("ljmpl %0, $x%=; x%=:" : : "I"(cs))
#define load_ds(ds) __asm__ volatile ("movw %%ax, %%ds"      : : "a"(ds))
#define load_es(es) __asm__ volatile ("movw %%ax, %%es"      : : "a"(es))
#define load_fs(fs) __asm__ volatile ("movw %%ax, %%fs"      : : "a"(fs))
#define load_gs(gs) __asm__ volatile ("movw %%ax, %%gs"      : : "a"(gs))
#define load_ss(ss) __asm__ volatile ("movw %%ax, %%ss"      : : "a"(ss))

#endif /* __ASSEMBLY__ */

#endif /* __X86_DESC_H */
