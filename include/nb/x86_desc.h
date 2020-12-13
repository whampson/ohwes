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

/* Segment Selectors */
#define KERNEL_CS               0x10    /* Kernel Code Segment */
#define KERNEL_DS               0x18    /* Kernel Data Segment */
#define USER_CS                 0x23    /* User-space Code Segment */
#define USER_DS                 0x2B    /* User-space Data Segment */
#define TSS_SEG                 0x30    /* TSS Segment */
#define LDT_SEG                 0x38    /* LDT Segment */

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
typedef union segsel
{
    struct fields
    {
        uint16_t rpl    : 2;        /* requested privilege level */
        uint16_t ti     : 1;        /* table type; 0 = GDT, 1 = LDT */
        uint16_t index  : 13;       /* descriptor table index */
    } fields;
    uint16_t value;
} seg_sel_t;

/**
 * Segment Descriptor
 */
typedef union segdesc
{
    struct gdt_ldt
    {
        uint64_t limit_lo   : 16;   /* Size (low bits) */
        uint64_t address_lo : 24;   /* Address (low bits) */
        uint64_t type       : 4;    /* Segment/Gate Type */
        uint64_t s          : 1;    /* Descriptor Type; 0 = System, 1 = Code/Data */
        uint64_t dpl        : 2;    /* Descriptor Privilege Level */
        uint64_t p          : 1;    /* Present Bit */
        uint64_t limit_hi   : 4;    /* Size (high bits) */
        uint64_t avl        : 1;    /* Available for Use */
        uint64_t l          : 1;    /* 64-bit Code Segment */
        uint64_t db         : 1;    /* Default Op/Stack size, Upper Bound */
        uint64_t g          : 1;    /* Granularity; 0 = Byte, 1 = 4K Page */
        uint64_t addesss_hi : 8;    /* Address (low bits) */
    } gdt_ldt;
    struct tss
    {
        uint64_t limit_lo   : 16;
        uint64_t address_lo : 24;
        uint64_t type       : 4;
        uint64_t            : 1;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t limit_hi   : 4;
        uint64_t avl        : 1;
        uint64_t            : 1;
        uint64_t            : 1;
        uint64_t g          : 1;
        uint64_t addesss_hi : 8;
    } tss;
    struct call_gate
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
    struct task_gate
    {
        uint64_t            : 16;
        uint64_t tss_segsel : 16;
        uint64_t            : 8;
        uint64_t type       : 4;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t            : 16;
    } task_gate;
    struct int_gate
    {
        uint64_t offset_lo  : 16;
        uint64_t segsel     : 16;
        uint64_t            : 8;
        uint64_t type       : 4;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t offset_hi  : 16;
    } int_gate;
    struct trap_gate
    {
        uint64_t offset_lo  : 16;
        uint64_t segsel     : 16;
        uint64_t            : 8;
        uint64_t type       : 4;
        uint64_t dpl        : 2;
        uint64_t p          : 1;
        uint64_t offset_hi  : 16;
    } trap_gate;
    uint64_t value;
} segdesc_t;

/**
 * Descriptor Register (GDTR/IDTR)
 */
typedef union descreg
{
    struct fields
    {
        uint64_t limit  : 16;
        uint64_t base   : 32;
        uint64_t        : 16;
    } fields;
    uint64_t value;
} descreg_t;

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
};

/**
 * Load the Global Descriptor Table Register.
 *
 * @param descreg a descreg_t containing the GDT base and limit
 */
#define lgdt(descreg)       \
__asm__ volatile (          \
    "lgdt %0"               \
    :                       \
    : "m"(descreg)          \
    : "memory", "cc"        \
);

/**
 * Load the Interrupt Descriptor Table Register.
 *
 * @param descreg a descreg_t containing the IDT base and limit
 */
#define lidt(descreg)       \
__asm__ volatile (          \
    "lidt %0"               \
    :                       \
    : "m"(descreg)          \
    : "memory", "cc"        \
);

/**
 * Load the Local Descriptor Table Register.
 *
 * @param selector segment selector for the LDT
 */
#define lldt(selector)      \
__asm__ volatile (          \
    "lldt %w0"              \
    :                       \
    : "r"(selector)         \
    : "memory", "cc"        \
);

/**
 * Load the Task Register.
 *
 * @param selector segment selector for the TSS
 */
#define ltr(selector)       \
__asm__ volatile (          \
    "ltr %w0"               \
    :                       \
    : "r"(selector)         \
    : "memory", "cc"        \
);

#endif /* __ASSEMBLY__ */

#endif /* __X86_DESC_H */
