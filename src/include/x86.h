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
 *         File: include/hw/x86.h
 *      Created: December 12, 2020
 *       Author: Wes Hampson
 *
 * Structure definitions and constants for x86-family CPUs.
 * See Intel IA-32 Software Developer's Manual Volume 3A for more information.
 * =============================================================================
 */

#ifndef X86_H
#define X86_H

/**
 * System Descriptor Types
 */
#define DESC_SYS_TSS16      0x01    /* Task State Segment (16-bit) */
#define DESC_SYS_LDT        0x02    /* Local Descriptor Table */
#define DESC_SYS_CALL16     0x04    /* Call Gate (16-bit) */
#define DESC_SYS_TASK       0x05    /* Task Gate */
#define DESC_SYS_INTR16     0x06    /* Interrupt Gate (16-bit) */
#define DESC_SYS_TRAP16     0x07    /* Trap Gate (16-bit) */
#define DESC_SYS_TSS32      0x09    /* Task State Segment (32-bit) */
#define DESC_SYS_CALL32     0x0C    /* Call Gate (32-bit) */
#define DESC_SYS_INTR32     0x0E    /* Interrupt Gate (32-bit) */
#define DESC_SYS_TRAP32     0x0F    /* Trap Gate (32-bit) */

/*
 * Memory Descriptor Types
 *
 * Below are notes on Conforming and Expand-Down segments, from the
 * Intel Software Developer's Manual, Volume 3A (section 3.4.5).
 *
 * The processor uses the segment limit in two different ways, depending on
 * whether the segment is an expand-up or an expand-down segment. See Section
 * 3.4.5.1, “Code- and Data-Segment Descriptor Types”, for more information
 * about segment types. For expand-up segments, the offset in a logical address
 * can range from 0 to the segment limit. Offsets greater than the segment limit
 * generate general-protection exceptions (#GP, for all segments other than SS)
 * or stack-fault excep- tions (#SS for the SS segment). For
 * expand-down segments, the segment limit has the reverse function; the offset
 * can range from the segment limit plus 1 to FFFFFFFFH or FFFFH, depending on
 * the setting of the B flag. Offsets less than or equal to the segment limit
 * generate general-protection exceptions or stack-fault exceptions. Decreasing
 * the value in the segment limit field for an expand- down segment allocates
 * new memory at the bottom of the segment's address space, rather than at the
 * top. IA-32 architecture stacks always grow downwards, making this mechanism
 * convenient for expandable stacks. (p. 3-10)
 *
 * Code segments can be either conforming or nonconforming. A transfer of
 * execution into a more-privileged conforming segment allows execution to
 * continue at the current privilege level. A transfer into a nonconforming
 * segment at a different privilege level results in a general-protection
 * exception (#GP), unless a call gate or task gate is used (see Section 5.8.1,
 * “Direct Calls or Jumps to Code Segments”, for more information on conforming
 * and nonconforming code segments. (p. 3-13)
 */
#define DESC_MEM_DATA_R     0x00    /* Read-Only */
#define DESC_MEM_DATA_RA    0x01    /* Read-Only, Accessed */
#define DESC_MEM_DATA_RW    0x02    /* Read/Write */
#define DESC_MEM_DATA_RWA   0x03    /* Read/Write, Accessed */
#define DESC_MEM_DATA_RE    0x04    /* Read-Only, Expand-Down */
#define DESC_MEM_DATA_REA   0x05    /* Read-Only, Expand-Down, Accessed */
#define DESC_MEM_DATA_RWE   0x06    /* Read/Write, Expand-Down */
#define DESC_MEM_DATA_RWEA  0x07    /* Read/Write, Expand-Down, Accessed */
#define DESC_MEM_CODE_X     0x08    /* Execute-Only */
#define DESC_MEM_CODE_XA    0x09    /* Execute-Only, Accessed */
#define DESC_MEM_CODE_XR    0x0A    /* Execute/Read */
#define DESC_MEM_CODE_XRA   0x0B    /* Execute/Read, Accessed */
#define DESC_MEM_CODE_XC    0x0C    /* Execute-Only, Conforming */
#define DESC_MEM_CODE_XCA   0x0D    /* Execute-Only, Conforming, Accessed */
#define DESC_MEM_CODE_XRC   0x0E    /* Execute/Read, Conforming */
#define DESC_MEM_CODE_XRCA  0x0F    /* Execute/Read, Conforming */

#define LIMIT_MAX           0xFFFFF /* Maximum Segment Descriptor Limit */

#define DESC_SIZE           8
#define TSS_SIZE            108

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Descriptor Privilege Levels
 */
enum Dpl
{
    KernelMode = 0,
    UserMode = 3
};

/*
 * Segment Descriptor
 * A Segment Descriptor is a data structure in the GDT or LDT that provides the
 * CPU with access control, status, and location/size information about a
 * memory segment, interrupt handler, system task, or program control transfer
 * between different privilege levels.
 *
 * - Segment Descriptor: Defines access control, status, location, and size
 *      information for a memory segment or a system segment.
 * - TSS Descriptor: Defines access control, status, location, and size
 *      information for a Task State Segment.
 * - Task Gate Descriptor: Provides an indirect, protected reference to a task.
 * - Call Gate Descriptor: Facilitates controlled transfers of program control
 *      between different privilege levels.
 * - Interrupt Gate Descriptor: Contains a far-pointer to an interrupt or
 *      exception handler. The [IF] flag is cleared when using an Interrupt
 *      Gate, effectively disabling interrupts for the duration of handler
 *      execution.
 * - Trap Gate Descriptor: Contains a far-pointer to an interrupt or exception
 *      handler. The [IF] flag remains unchanged when using a Trap Gate.
 */
typedef struct SegDesc_
{
    union {
        struct MemDesc {        /* Code/Data Segment Descriptor */
            uint64_t limitLo    : 16;   /* Segment Limit[15:0] */
            uint64_t baseLo     : 24;   /* Segment Base[23:0] */
            uint64_t type       : 4;    /* Segment Descriptor Type; one of DESC_MEM_* */
            uint64_t s          : 1;    /* System Flag; set to 1 for code/data segment */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present in Memory */
            uint64_t limitHi    : 4;    /* Segment Limit[19:16] */
            uint64_t avl        : 1;    /* Available for Software to Use */
            uint64_t            : 1;    /* Reserved; set to 0 */
            uint64_t db         : 1;    /* (Code/Stack) 0 = 16 bit, 1 = 32-bit; (Expand-Down) 0 = 64K, 1 = 4G */
            uint64_t g          : 1;    /* Granularity; 0 = Byte, 1 = 4K Page */
            uint64_t baseHi     : 8;    /* Segment Base[31:24] */
        } seg;
        struct TssDesc {        /* TSS Descriptor */
            uint64_t limitLo    : 16;   /* TSS Limit[15:0] */
            uint64_t baseLo     : 24;   /* TSS Base[bits 23:0] */
            uint64_t type       : 4;    /* Descriptor Type; one of DESC_SYS_TSS* */
            uint64_t            : 1;    /* Reserved; set to 0 */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present in Memory */
            uint64_t limitHi    : 4;    /* TSS Limit[19:16] */
            uint64_t avl        : 1;    /* Available for Software to Use */
            uint64_t            : 1;    /* Reserved; set to 0 */
            uint64_t            : 1;    /* Reserved; set to 0 */
            uint64_t g          : 1;    /* Granularity; 0 = Byte, 1 = 4K Page */
            uint64_t baseHi     : 8;    /* TSS Base[31:24] */
        } tss;
        struct TaskDesc {       /* Task Gate Descriptor */
            uint64_t            : 16;   /* Reserved; set to 0 */
            uint64_t tssSegSel  : 16;   /* TSS Segment Selector */
            uint64_t            : 8;    /* Reserved; set to 0 */
            uint64_t type       : 4;    /* Descriptor Type; DESC_SYS_TASK */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present in Memory */
            uint64_t            : 16;   /* Reserved; set to 0 */
        } task;
        struct CallDesc {       /* Call Gate Descriptor */
            uint64_t offsetLo   : 16;   /* Entry Point[15:0] */
            uint64_t segSel     : 16;   /* Code Segment Selector */
            uint64_t paramCount : 5;    /* Number of Stack Parameters */
            uint64_t            : 3;    /* Reserved; set to 0 */
            uint64_t type       : 4;    /* Descriptor Type; one of DESC_SYS_CALL* */
            uint64_t            : 1;    /* Reserved; set to 0 */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present in Memory */
            uint64_t offsetHi   : 16;   /* Entry Point[31:16] */
        } call;
        struct IntrDesc {       /* Interrupt Gate Descriptor */
            uint64_t offsetLo   : 16;   /* Entry Point[15:0] */
            uint64_t segSel     : 16;   /* Code Segment Selector */
            uint64_t            : 8;    /* Reserved; set to 0 */
            uint64_t type       : 4;    /* Descriptor Type; one of DESC_SYS_INT* */
            uint64_t            : 1;    /* Reserved; set to 0 */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present in Memory */
            uint64_t offsetHi   : 16;   /* Entry Point[31:16] */
        } intr;
        struct TrapDesc {       /* Trap Gate Descriptor */
            uint64_t offsetLo   : 16;   /* Entry Point (bis 15:0) */
            uint64_t segSel     : 16;   /* Code Segment Selector */
            uint64_t            : 8;    /* Reserved; Set to 0 */
            uint64_t type       : 4;    /* Descriptor Type; DESC_SYS_INT* */
            uint64_t            : 1;    /* Reserved; Set to 0 */
            uint64_t dpl        : 2;    /* Descriptor Privilege Level */
            uint64_t p          : 1;    /* Present */
            uint64_t offsetHi   : 16;   /* Entry Point (bits 31:16) */
        } trap;
        uint64_t _value;                /* Aggregate value */
    };
} SegDesc;
_Static_assert(sizeof(SegDesc) == DESC_SIZE, "sizeof(SegDesc) == DESC_SIZE");

/*
 * Segment Selector
 * A Segment Selector is 16-bit identifier for a segment that points to the
 * Segment Descriptor that defines the segment. Segment Selectors are loaded
 * into the segment registers (CS, DS, ES, FS, GS, SS).
 */
typedef struct SegSel_
{
    union {
        struct {
            uint16_t rpl    : 2;        /* Requested Privilege Level */
            uint16_t ti     : 1;        /* Table Type; 0 = GDT, 1 = LDT */
            uint16_t index  : 13;       /* Descriptor Table Index */
        };
        uint16_t _value;                /* Aggregate Value */
    };
} SegSel;
_Static_assert(sizeof(SegSel) == 2, "sizeof(SegSel) == 2");

/*
 * Descriptor Register
 * Represents the data structure supplied in the LGDT and LIDT instructions for
 * loading the GDTR and IDTR registers respectively. The limit and base fields
 * together are referred to as the "pseudo-descriptor".
 *
 * The alignment on this structure is tricky; the limit field should be aligned
 * to an odd-word address (address MOD 4 equals 2). You may need to stick an
 * '__align(2)' before instantiating this.
 */
typedef struct DescReg_
{
    uint32_t        :16;    /* (pad) */
    uint32_t limit  :16;    /* Descriptor Table Limit */
    uint32_t base;          /* Descriptor Table Base */
} DescReg;
_Static_assert(sizeof(DescReg) == 8, "sizeof(DescReg) == 8");

/*
 * Task State Segment
 * The Task State Segment (TSS) contains processor state information needed to
 * save and restore a task.
 */
struct Tss
{
    uint16_t prevTask;
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
    uint16_t ldtSegSel;
    uint16_t _reserved10;
    uint16_t debugTrap : 1;
    uint16_t _reserved11 : 15;
    uint16_t ioMapBase;
    uint32_t ssp;
};
_Static_assert(sizeof(struct Tss) == TSS_SIZE, "sizeof(struct Tss) == TSS_SIZE");

/**
 * Gets a pointer to a Segment Descriptor from a descriptor table.
 *
 * @param table a pointer to the descriptor table
 * @param sel the segment selector
 * @return a pointer to the segment descriptor specified by the segment selector
 */
#define GetDescPtr(table,sel) \
    (((SegDesc *) (table)) + (((uint32_t) (sel)) / sizeof(SegDesc)))

/**
 * Configures a Segment Descriptor as a 32-bit Code or Data Segment. Code/Data
 * Segment descriptors go in the GDT or LDT.
 *
 * @param pDesc segment descriptor pointer
 * @param dpl segment descriptor privilege level
 * @param base segment base address
 * @param limit segment limit (size - 1)
 * @param type segment type (one of DESC_MEM_CODE_* or DESC_MEM_DATA_*)
 *
 */
static inline void MakeSegDesc(SegDesc *pDesc, int dpl, int base, int limit, int type)
{
    pDesc->_value = 0;
    pDesc->seg.type = type;
    pDesc->seg.dpl = dpl;
    pDesc->seg.s = 1;       // 1 = memory descriptor (code/data)
    pDesc->seg.db = 1;      // 1 = 32-bit
    pDesc->seg.baseLo = ((base) & 0x00FFFFFF);
    pDesc->seg.baseHi = ((base) & 0xFF000000) >> 24;
    pDesc->seg.limitLo = ((limit) & 0x0FFFF);
    pDesc->seg.limitHi = ((limit) & 0xF0000) >> 16;
    pDesc->seg.g = 1;       // 1 = 4K page granularity
    pDesc->seg.p = 1;       // 1 = present in memory
}

/**
 * Configures a System Segment Descriptor as a 32-bit LDT Segment. LDT Segment
 * descriptors go in the GDT.
 *
 * @param pDesc LDT segment descriptor pointer
 * @param dpl LDT descriptor privilege level
 * @param base LDT base address
 * @param limit LDT limit (size - 1)
 */
static inline void MakeLdtDesc(SegDesc *pDesc, int dpl, int base, int limit)
{
    pDesc->_value = 0;
    pDesc->seg.type = DESC_SYS_LDT;
    pDesc->seg.dpl = dpl;
    pDesc->seg.s = 0;       // 0 = system descriptor
    pDesc->seg.db = 1;      // 1 = 32-bit
    pDesc->seg.baseLo = ((base) & 0x00FFFFFF);
    pDesc->seg.baseHi = ((base) & 0xFF000000) >> 24;
    pDesc->seg.limitLo = ((limit) & 0x0FFFF);
    pDesc->seg.limitHi = ((limit) & 0xF0000) >> 16;
    pDesc->seg.g = 0;       // 0 = byte granularity
    pDesc->seg.p = 1;       // 1 = present in memory
}

/**
 * Configures a System Segment Descriptor as a 32-bit TSS Descriptor. TSS
 * Segment descriptors go in the GDT.
 *
 * @param pDesc TSS segment descriptor pointer
 * @param dpl TSS descriptor privilege level
 * @param base TSS base address
 * @param limit TSS limit (size - 1)
 */
static inline void MakeTssDesc(SegDesc *pDesc, int dpl, int base, int limit)
{
    pDesc->_value = 0;
    pDesc->tss.type = DESC_SYS_TSS32;
    pDesc->tss.dpl = dpl;
    pDesc->tss.baseLo = ((base) & 0x00FFFFFF);
    pDesc->tss.baseHi = ((base) & 0xFF000000) >> 24;
    pDesc->tss.limitLo = ((limit) & 0x0FFFF);
    pDesc->tss.limitHi = ((limit) & 0xF0000) >> 16;
    pDesc->tss.g = 0;    // 0 = byte granularity
    pDesc->tss.p = 1;    // 1 = present in memory
}

/**
 * Configures a System Segment Descriptor as a Task Gate.
 * A Task Gate descriptor provides an indirect, protected reference to a task. A
 * Task Gate is similar to a Call Gate, except that it provides access (through
 * a segment selector) to a TSS rather than a code segment. Task Gate
 * descriptors go in the IDT.
 *
 * @param pDesc Task Gate segment descriptor pointer
 * @param tssSegSel TSS Segment Selector
 * @param dpl task gate privilege level
 */
static inline void MakeTaskDesc(SegDesc *pDesc, int tssSegSel, int dpl)
{
    pDesc->_value = 0;
    pDesc->task.type = DESC_SYS_TASK;
    pDesc->task.tssSegSel = tssSegSel;
    pDesc->task.dpl = dpl;
    pDesc->task.p = 1;  // 1 = present in memory
}

/**
 * Configures a System Segment Descriptor as a 32-bit Call Gate.
 * Call Gates facilitate controlled transfers of program control between
 * different privilege levels in a non-interrupt context (i.e. using the CALL
 * instruction). Call Gates descriptors go in the LDT.
 *
 * @param pDesc Call Gate segment descriptor pointer
 * @param segSel call handler code segment selector
 * @param dpl Call Gate descriptor privilege level
 * @param paramCount number of stack parameters
 * @param pHandler a pointer to the call handler function
 */
static inline void MakeCallDesc(SegDesc *pDesc, int segSel, int dpl, int paramCount, void *pHandler)
{
    pDesc->_value = 0;
    pDesc->call.type = DESC_SYS_CALL32;
    pDesc->call.segSel = segSel;
    pDesc->call.dpl = dpl;
    pDesc->call.paramCount = paramCount;
    pDesc->call.offsetLo = ((uint32_t) pHandler) & 0xFFFF;
    pDesc->call.offsetHi = ((uint32_t) pHandler) >> 16;
    pDesc->call.p = (pHandler != NULL);
}

/**
 * Configures a System Segment Descriptor as a 32-bit Interrupt Gate.
 * An Interrupt Gate is like a Call Gate, except it clears [IF] after EFLAGS is
 * pushed, preventing other interrupts from interfering with the current
 * handler. Interrupt Gate descriptors go in the IDT.
 *
 * @param pDesc Interrupt Gate segment descriptor pointer
 * @param segSel interrupt handler code segment selector
 * @param dpl interrupt Gate descriptor privilege level
 * @param pHandler a pointer to the interrupt handler function
 */
static inline void MakeIntrDesc(SegDesc *pDesc, int segSel, int dpl, void *pHandler)
{
    pDesc->_value = 0;
    pDesc->intr.type = DESC_SYS_INTR32;
    pDesc->intr.segSel = segSel;
    pDesc->intr.dpl = dpl;
    pDesc->intr.offsetLo = ((uint32_t) pHandler) & 0xFFFF;
    pDesc->intr.offsetHi = ((uint32_t) pHandler) >> 16;
    pDesc->intr.p = (pHandler != NULL);
}

/**
 * Configures a System Segment Descriptor as a 32-bit Trap Gate.
 * A Trap Gate is like an Interrupt Gate, except it does not clear [IF], which
 * does not prevent other interrupts from interfering with the handler. Trap
 * Gate descriptors go in the IDT.
 *
 * @param desc Trap Gate segment descriptor pointer
 * @param sel trap handler code segment selector
 * @param pl trap handler privilege level
 * @param handler a pointer to the trap handler function
 */
static inline void MakeTrapDesc(SegDesc *pDesc, int segSel, int dpl, void *pHandler)
{
    pDesc->_value = 0;
    pDesc->trap.type = DESC_SYS_TRAP32;
    pDesc->trap.segSel = segSel;
    pDesc->trap.dpl = dpl;
    pDesc->trap.offsetLo = ((uint32_t) pHandler) & 0xFFFF;
    pDesc->trap.offsetHi = ((uint32_t) pHandler) >> 16;
    pDesc->trap.p = (pHandler != NULL);
}

/**
 * Loads the Global Descriptor Table Register (GDTR).
 *
 * @param pDesc a pointer to the "pseudo-descriptor" that contains the limit and
 *              base address of the LDT; the alignment on pDesc structure is
 *              tricky; the limit field should be aligned to an odd-word address
 *              (address MOD 4 equals 2)
 */
#define lgdt(pDesc)             \
__asm__ volatile (              \
    "lgdt %0"                   \
    :                           \
    : "m"(pDesc)                \
    :                           \
)

/**
 * Loads the Interrupt Descriptor Table Register (IDTR).
 *
 * @param pDesc a pointer to the "pseudo-descriptor" that contains the limit and
 *              base address of the IDT; the alignment on pDesc structure is
 *              tricky; the limit field should be aligned to an odd-word address
 *              (address MOD 4 equals 2)
 */
#define lidt(pDesc)         \
__asm__ volatile (          \
    "lidt %0"               \
    :                       \
    : "m"(pDesc)            \
    :                       \
)

/**
 * Loads the Local Descriptor Table Register (LDTR).
 *
 * @param segSel segment selector for the LDT
 */
#define lldt(segSel)        \
__asm__ volatile (          \
    "lldt %w0"              \
    :                       \
    : "r"(segSel)           \
    :                       \
)

/**
 * Loads the Task Register (TR).
 *
 * @param segSel segment selector for the TSS
 */
#define ltr(segSel)         \
__asm__ volatile (          \
    "ltr %w0"               \
    :                       \
    : "r"(segSel)           \
    :                       \
)

#define LoadCs(cs) __asm__ volatile ("ljmpl %0, $x%=; x%=:" : : "I"(cs))
#define LoadDs(ds) __asm__ volatile ("movw %%ax, %%ds"      : : "a"(ds))
#define LoadEs(es) __asm__ volatile ("movw %%ax, %%es"      : : "a"(es))
#define LoadFs(fs) __asm__ volatile ("movw %%ax, %%fs"      : : "a"(fs))
#define LoadGs(gs) __asm__ volatile ("movw %%ax, %%gs"      : : "a"(gs))
#define LoadSs(ss) __asm__ volatile ("movw %%ax, %%ss"      : : "a"(ss))

#endif /* __ASSEMBLER__ */

#endif /* X86_H */