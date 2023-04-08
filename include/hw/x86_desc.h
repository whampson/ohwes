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
 *         File: include/hw/x86_desc.h
 *      Created: December 12, 2020
 *       Author: Wes Hampson
 *
 * Descriptor definitions for x86-family CPUs.
 * See Intel IA-32 Software Developer's Manual, Volume 3 for more information on
 * each structure.
 * =============================================================================
 */

#ifndef __X86_DESC_H
#define __X86_DESC_H

#include <stdint.h>
#include <string.h>

/* System Descriptor Types */
#define DESC_TYPE_SYS_TSS16     0x01    /* Task State Segment (16-bit) */
#define DESC_TYPE_SYS_LDT       0x02    /* Local Descriptor Table */
#define DESC_TYPE_SYS_CALL16    0x04    /* Call Gate (16-bit) */
#define DESC_TYPE_SYS_TASK      0x05    /* Task Gate */
#define DESC_TYPE_SYS_INTR16    0x06    /* Interrupt Gate (16-bit) */
#define DESC_TYPE_SYS_TRAP16    0x07    /* Trap Gate (16-bit) */
#define DESC_TYPE_SYS_TSS32     0x09    /* Task State Segment (32-bit) */
#define DESC_TYPE_SYS_CALL32    0x0C    /* Call Gate (32-bit) */
#define DESC_TYPE_SYS_INTR32    0x0E    /* Interrupt Gate (32-bit) */
#define DESC_TYPE_SYS_TRAP32    0x0F    /* Trap Gate (32-bit) */

/* Code- and Data-Segment Descriptor Types */
#define DESC_TYPE_DATA_R        0x00    /* Read-Only */
#define DESC_TYPE_DATA_RA       0x01    /* Read-Only, Accessed */
#define DESC_TYPE_DATA_RW       0x02    /* Read/Write */
#define DESC_TYPE_DATA_RWA      0x03    /* Read/Write, Accessed */
#define DESC_TYPE_DATA_RE       0x04    /* Read-Only, Expand-Down */
#define DESC_TYPE_DATA_REA      0x05    /* Read-Only, Expand-Down, Accessed */
#define DESC_TYPE_DATA_RWE      0x06    /* Read/Write, Expand-Down */
#define DESC_TYPE_DATA_RWEA     0x07    /* Read/Write, Expand-Down, Accessed */
#define DESC_TYPE_CODE_X        0x08    /* Execute-Only */
#define DESC_TYPE_CODE_XA       0x09    /* Execute-Only, Accessed */
#define DESC_TYPE_CODE_XR       0x0A    /* Execute/Read */
#define DESC_TYPE_CODE_XRA      0x0B    /* Execute/Read, Accessed */
#define DESC_TYPE_CODE_XC       0x0C    /* Execute-Only, Conforming */
#define DESC_TYPE_CODE_XCA      0x0D    /* Execute-Only, Conforming, Accessed */
#define DESC_TYPE_CODE_XRC      0x0E    /* Execute/Read, Conforming */
#define DESC_TYPE_CODE_XRCA     0x0F    /* Execute/Read, Conforming */

#define LIMIT_MAX               0xFFFFF /* Maximum Segment Descriptor Limit */

/**
 * x86 Descriptor Structure
 * A Descriptor is a data structure that provides the CPU with access control,
 * status, and location/size information about a code or data segment,
 * interrupt handler, system task, or program control transfer between
 * different privilege levels.
 *
 * - Segment Descriptor: Defines access control, status, location, and size
 *      information for a code or data segment, or a Local Descriptor Table.
 * - TSS Descriptor: Defines access control, status, location, and size
 *      information for a Task State Segment.
 * - Task Gate Descriptor: Provides an indirect, protected reference to a task.
 * - Call Gate Descriptor: Facilitates controlled transfers of program control
 *      between different privilege levels.
 * - Interrupt Gate Descriptor: Contains a far-pointer to an interrupt or
 *      exception handler. The IF flag is cleared when using an Interrupt Gate,
 *      effectively disabling interrupts for the duration of handler execution.
 * - Trap Gate Descriptor: Contains a far-pointer to an interrupt or exception
 *      handler. The IF flag remains unchanged when using a Trap Gate.
 */
struct x86_desc
{
    union
    {
        struct seg_desc {       /* Segment Descriptor */
            uint64_t limit_lo   : 16;   /* Segment Limit (bits 15:0) */
            uint64_t base_lo    : 24;   /* Segment Base (bits 23:0) */
            uint64_t type       : 4;    /* Descriptor Type */
            uint64_t s          : 1;    /* Descriptor Class; 0 = System, 1 = Code/Data */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present */
            uint64_t limit_hi   : 4;    /* Segment Limit (bits 19:16) */
            uint64_t avl        : 1;    /* Available for Software to Use */
            uint64_t            : 1;    /* Reserved; Set to 0 */
            uint64_t db         : 1;    /* 0 = 16 bit, 1 = 32-bit */
            uint64_t g          : 1;    /* Granularity; 0 = Byte, 1 = 4K Page */
            uint64_t base_hi    : 8;    /* Segment Base (bits 31:24) */
        } seg;
        struct tss_desc {       /* TSS Descriptor */
            uint64_t limit_lo   : 16;   /* TSS Limit (bits 15:0) */
            uint64_t base_lo    : 24;   /* TSS Base (bits 23:0) */
            uint64_t type       : 4;    /* Descriptor Type; DESC_TYPE_SYS_TSS* */
            uint64_t            : 1;    /* Reserved; Set to 0 */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present */
            uint64_t limit_hi   : 4;    /* TSS Limit (bits 19:16) */
            uint64_t avl        : 1;    /* Available for Software to Use */
            uint64_t            : 1;    /* Reserved; Set to 0 */
            uint64_t            : 1;    /* Reserved; Set to 0 */
            uint64_t g          : 1;    /* Granularity; 0 = Byte, 1 = 4K Page */
            uint64_t base_hi    : 8;    /* TSS Base (bits 31:24) */
        } tss;
        struct task_desc {      /* Task Gate Descriptor */
            uint64_t            : 16;   /* Reserved; Set to 0 */
            uint64_t tss_seg_sel : 16;   /* TSS Segment Selector */
            uint64_t            : 8;    /* Reserved; Set to 0 */
            uint64_t type       : 4;    /* Descriptor Type; DESC_TYPE_SYS_TASK */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present */
            uint64_t            : 16;   /* Reserved; Set to 0 */
        } task;
        struct call_desc {      /* Call Gate Descriptor */
            uint64_t offset_lo  : 16;   /* Entry Point (bits 15:0) */
            uint64_t seg_sel     : 16;   /* Code Segment Selector */
            uint64_t param_count: 5;    /* Number of Stack Parameters */
            uint64_t            : 3;    /* Reserved; Set to 0 */
            uint64_t type       : 4;    /* Descriptor Type; DESC_TYPE_SYS_CALL* */
            uint64_t            : 1;    /* Reserved; Set to 0 */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present */
            uint64_t offset_hi  : 16;   /* Entry Point (bits 31:16) */
        } call;
        struct intr_desc {      /* Interrupt Gate Descriptor */
            uint64_t offset_lo  : 16;   /* Entry Point (bis 15:0) */
            uint64_t seg_sel     : 16;   /* Code Segment Selector */
            uint64_t            : 8;    /* Reserved; Set to 0 */
            uint64_t type       : 4;    /* Descriptor Type; DESC_TYPE_SYS_INT* */
            uint64_t            : 1;    /* Reserved; Set to 0 */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present */
            uint64_t offset_hi  : 16;   /* Entry Point (bits 31:16) */
        } intr;
        struct trap_desc {      /* Trap Gate Descriptor */
            uint64_t offset_lo  : 16;   /* Entry Point (bis 15:0) */
            uint64_t seg_sel     : 16;   /* Code Segment Selector */
            uint64_t            : 8;    /* Reserved; Set to 0 */
            uint64_t type       : 4;    /* Descriptor Type; DESC_TYPE_SYS_INT* */
            uint64_t            : 1;    /* Reserved; Set to 0 */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present */
            uint64_t offset_hi  : 16;   /* Entry Point (bits 31:16) */
        } trap;
        uint64_t _value;                /* Aggregate value */
    };
};
_Static_assert(sizeof(struct x86_desc) == 8, "sizeof(struct x86_desc)");

/**
 * Segment Selector
 * Points to a Segment Descriptor that defines a segment.
 */
struct seg_sel
{
    union {
        struct {
            uint16_t rpl    : 2;        /* Requested Privilege Level */
            uint16_t ti     : 1;        /* Table Type; 0 = GDT, 1 = LDT */
            uint16_t index  : 13;       /* Descriptor Table Index */
        };
        uint16_t _value;                /* Aggregate Value */
    };
};
_Static_assert(sizeof(struct seg_sel) == 2, "sizeof(struct seg_sel)");

/**
 * Descriptor Register
 * Represents the data structure supplied in the LGDT and LIDT instructions
 * specifying the location and size of the GDT and IDT respectively.
 */
struct desc_reg
{
    union {
        struct {
            uint64_t limit  : 16;   /* Descriptor Table Limit */
            uint64_t base   : 32;   /* Descriptor Table Base */
            uint64_t        : 16;   /* (align)*/
        };
        uint64_t _value;            /* Aggregate Value */
    };
};
_Static_assert(sizeof(struct desc_reg) == 8, "sizeof(struct desc_reg)");

#define get_aligned_desc_reg_ptr(desc_reg)    (&(desc_reg.limit))

/**
 * Task State Segment
 * Processor state information needed to save and restore a task.
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
    uint16_t ldt_seg;
    uint16_t _reserved10;
    uint16_t debug_trap : 1;
    uint16_t _reserved11 : 15;
    uint16_t io_map_base;
    uint32_t ssp;
};
_Static_assert(sizeof(struct tss) == 108, "sizeof(struct tss)");

/**
 * Gets a pointer to a Segment Descriptor from a descriptor table.
 *
 * @param table a pointer to the descriptor table
 * @param sel the segment selector
 * @return a pointer to the segment descriptor specified by the segment selector
 */
#define get_seg_desc(table,sel) \
    (((struct x86_desc *) (table)) + (((uint32_t) (sel)) / sizeof(struct x86_desc)))

/**
 * Configures a Segment Descriptor as a 32-bit Code or Data Segment.
 *
 * @param desc a pointer to an x86_desc
 * @param pl segment privilege level
 * @param en descriptor enable bit (0 = disable, 1 = enable)
 * @param base segment base address
 * @param lim segment limit (size - 1)
 * @param gr segment granularity (0 = byte, 1 = 4K page)
 * @param typ segment type (one of DESC_TYPE_CODE_* or DESC_TYPE_DATA_*)
 *
 */
#define set_seg_desc(desc,pl,en,base,lim,gr,typ)                            \
do {                                                                        \
    desc->_value = 0;                                                       \
    desc->seg.type = typ;                                                   \
    desc->seg.dpl = pl;                                                     \
    desc->seg.s = 1;                                                        \
    desc->seg.db = 1;                                                       \
    desc->seg.base_lo = ((base) & 0x00FFFFFF);                              \
    desc->seg.base_hi = ((base) & 0xFF000000) >> 24;                        \
    desc->seg.limit_lo = ((lim) & 0x0FFFF);                                 \
    desc->seg.limit_hi = ((lim) & 0xF0000) >> 16;                           \
    desc->seg.g = gr;                                                       \
    desc->seg.p = en;                                                       \
} while (0)

/**
 * Configures a Segment Descriptor as a 32-bit LDT Segment.
 *
 * @param desc a pointer to an x86_desc
 * @param pl LDT privilege level
 * @param en descriptor enable bit (0 = disable, 1 = enable)
 * @param base LDT base address
 * @param lim LDT limit (size - 1)
 * @param gr LDT granularity (0 = byte, 1 = 4K page)
 *
 */
#define set_ldt_desc(desc,pl,en,base,lim,gr)                                \
do {                                                                        \
    desc->_value = 0;                                                       \
    desc->seg.type = DESC_TYPE_SYS_LDT;                                     \
    desc->seg.dpl = pl;                                                     \
    desc->seg.s = 0;                                                        \
    desc->seg.db = 1;                                                       \
    desc->seg.base_lo = ((base) & 0x00FFFFFF);                              \
    desc->seg.base_hi = ((base) & 0xFF000000) >> 24;                        \
    desc->seg.limit_lo = ((lim) & 0x0FFFF);                                 \
    desc->seg.limit_hi = ((lim) & 0xF0000) >> 16;                           \
    desc->seg.g = gr;                                                       \
    desc->seg.p = en;                                                       \
} while (0)

/**
 * Configures a Segment Descriptor as a 32-bit TSS Descriptor.
 *
 * @param desc a pointer to an x86_desc
 * @param pl TSS privilege level
 * @param en descriptor enable bit (0 = disable, 1 = enable)
 * @param base TSS base address
 * @param lim TSS limit (size - 1)
 * @param gr TSS granularity (0 = byte, 1 = 4K page)
 */
#define set_tss_desc(desc,pl,en,base,lim,gr)                                \
do {                                                                        \
    desc->_value = 0;                                                       \
    desc->tss.type = DESC_TYPE_SYS_TSS32;                                   \
    desc->tss.dpl = pl;                                                     \
    desc->tss.base_lo = ((base) & 0x00FFFFFF);                              \
    desc->tss.base_hi = ((base) & 0xFF000000) >> 24;                        \
    desc->tss.limit_lo = ((lim) & 0x0FFFF);                                 \
    desc->tss.limit_hi = ((lim) & 0xF0000) >> 16;                           \
    desc->tss.g = gr;                                                       \
    desc->tss.p = en;                                                       \
} while (0)

/**
 * Configures a Segment Descriptor as a Task Gate.
 *
 * @param desc a pointer to an x86_desc
 * @param sel TSS Segment Selector
 * @param pl task gate privilege level
 * @param en descriptor enable bit (0 = disable, 1 = enable)
 */
#define set_task_desc(desc,sel,pl,en)                                       \
do {                                                                        \
    desc->_value = 0;                                                       \
    desc->task.type = DESC_TYPE_SYS_TASK;                                   \
    desc->task.tss_seg_sel = sel;                                            \
    desc->task.dpl = pl;                                                    \
    desc->task.p = en;                                                      \
} while (0)

/**
 * Configures a Segment Descriptor as a 32-bit Interrupt Gate.
 * An Interrupt Gate clears IF after EFLAGS is pushed, preventing other
 * interrupts from interfering with the current handler.
 *
 * @param desc a pointer to an x86_desc
 * @param sel interrupt handler code segment selector
 * @param pl interrupt handler privilege level
 * @param handler a pointer to the interrupt handler function
 */
#define set_intr_desc(desc,sel,pl,handler)                                  \
do {                                                                        \
    desc->_value = 0;                                                       \
    desc->intr.type = DESC_TYPE_SYS_INTR32;                                 \
    desc->intr.seg_sel = sel;                                                \
    desc->intr.dpl = pl;                                                    \
    desc->intr.offset_lo = ((uint32_t) handler) & 0xFFFF;                   \
    desc->intr.offset_hi = ((uint32_t) handler) >> 16;                      \
    desc->intr.p = handler != NULL;                                         \
} while (0)

/**
 * Configures a Segment Descriptor as a 32-bit Trap Gate.
 *
 * @param desc a pointer to an x86_desc
 * @param sel trap handler code segment selector
 * @param pl trap handler privilege level
 * @param handler a pointer to the trap handler function
 */
#define set_trap_desc(desc,sel,pl,handler)                                  \
do {                                                                        \
    desc->_value = 0;                                                       \
    desc->trap.type = DESC_TYPE_SYS_TRAP32;                                 \
    desc->trap.seg_sel = sel;                                                \
    desc->trap.dpl = pl;                                                    \
    desc->trap.offset_lo = ((uint32_t) handler) & 0xFFFF;                   \
    desc->trap.offset_hi = ((uint32_t) handler) >> 16;                      \
    desc->trap.p = handler != NULL;                                         \
} while (0)

/**
 * Loads the Global Descriptor Table Register.
 *
 * @param desc_reg a desc_reg structure containing the GDT base and limit
 */
#define lgdt(desc_reg)          \
__asm__ volatile (              \
    "lgdt %0"                   \
    :                           \
    : "m"(desc_reg)             \
    :                           \
)

/**
 * Loads the Interrupt Descriptor Table Register.
 *
 * @param desc_reg a desc_reg structure containing the IDT base and limit
 */
#define lidt(desc_reg)       \
__asm__ volatile (          \
    "lidt %0"               \
    :                       \
    : "m"(desc_reg)          \
    :                       \
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
    :                       \
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
    :                       \
)

#define load_cs(cs) __asm__ volatile ("ljmpl %0, $x%=; x%=:" : : "I"(cs))
#define load_ds(ds) __asm__ volatile ("movw %%ax, %%ds"      : : "a"(ds))
#define load_es(es) __asm__ volatile ("movw %%ax, %%es"      : : "a"(es))
#define load_fs(fs) __asm__ volatile ("movw %%ax, %%fs"      : : "a"(fs))
#define load_gs(gs) __asm__ volatile ("movw %%ax, %%gs"      : : "a"(gs))
#define load_ss(ss) __asm__ volatile ("movw %%ax, %%ss"      : : "a"(ss))

#endif /* __X86_DESC_H */
