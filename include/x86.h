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
 *         File: include/x86.h
 *      Created: December 12, 2020
 *       Author: Wes Hampson
 *
 * Structure definitions and constants for x86-family CPUs.
 * See Intel IA-32 Software Developer's Manual, Volume 3A for more information.
 * =============================================================================
 */

#ifndef __X86_H
#define __X86_H

#define SEGSEL_SIZE     2           // Size of a segment selector.
#define DESC_SIZE       8           // Size of a descriptor.
#define TSS_SIZE        108         // Size of a Task State Segment.

#define LIMIT_MAX       0xFFFFF     // Maximum value for descriptor "limit" field.

/**
 * System Descriptor Types in 32-bit mode.
 *
 * See Intel Software Developer's Manual, Volume 3A, section 3.5.
 */
#define DESCTYPE_TSS16          0x01    // 16-bit Task State Segment
#define DESCTYPE_LDT            0x02    // Local Descriptor Table
#define DESCTYPE_TSS16_BUSY     0x03    // 16-bit Task State Segment (Busy)
#define DESCTYPE_CALL16         0x04    // 16-bit Call Gate
#define DESCTYPE_TASK           0x05    // Task Gate
#define DESCTYPE_INTR16         0x06    // 16-bit Interrupt Gate
#define DESCTYPE_TRAP16         0x07    // 16-bit Trap Gate
#define DESCTYPE_TSS32          0x09    // 32-bit Task State Segment
#define DESCTYPE_TSS32_BUSY     0x0B    // 32-bit Task State Segment (Busy)
#define DESCTYPE_CALL32         0x0C    // 32-bit Call Gate
#define DESCTYPE_INTR32         0x0E    // 32-bit Interrupt Gate
#define DESCTYPE_TRAP32         0x0F    // 32-bit Trap Gate

/**
 * Segment Descriptor Types
 *
 * Below are notes on (Non-)Conforming and Expand-Down segments, from the
 * Intel Software Developer's Manual, Volume 3A, section 3.4.5:
 *
 * The processor uses the segment limit in two different ways, depending on
 * whether the segment is an expand-up or an expand-down segment. See Section
 * 3.4.5.1, “Code- and Data-Segment Descriptor Types”, for more information
 * about segment types. For expand-up segments, the offset in a logical address
 * can range from 0 to the segment limit. Offsets greater than the segment limit
 * generate general-protection exceptions (#GP, for all segments other than SS)
 * or stack-fault exceptions (#SS for the SS segment). For expand-down segments,
 * the segment limit has the reverse function; the offset can range from the
 * segment limit plus 1 to FFFFFFFFH or FFFFH, depending on the setting of the
 * B flag. Offsets less than or equal to the segment limit generate general-
 * protection exceptions or stack-fault exceptions. Decreasing the value in the
 * segment limit field for an expand- down segment allocates new memory at the
 * bottom of the segment's address space, rather than at the top. IA-32
 * architecture stacks always grow downwards, making this mechanism convenient
 * for expandable stacks. (p. 3-10)
 *
 * Code segments can be either conforming or nonconforming. A transfer of
 * execution into a more-privileged conforming segment allows execution to
 * continue at the current privilege level. A transfer into a nonconforming
 * segment at a different privilege level results in a general-protection
 * exception (#GP), unless a call gate or task gate is used (see Section 5.8.1,
 * “Direct Calls or Jumps to Code Segments”, for more information on conforming
 * and nonconforming code segments. (p. 3-13)
 */
#define DESCTYPE_DATA_R         0x00    // Data, Read-Only
#define DESCTYPE_DATA_RA        0x01    // Data, Read-Only, Accessed
#define DESCTYPE_DATA_RW        0x02    // Data, Read/Write
#define DESCTYPE_DATA_RWA       0x03    // Data, Read/Write, Accessed
#define DESCTYPE_DATA_RE        0x04    // Data, Read-Only, Expand-Down
#define DESCTYPE_DATA_REA       0x05    // Data, Read-Only, Expand-Down, Accessed
#define DESCTYPE_DATA_RWE       0x06    // Data, Read/Write, Expand-Down
#define DESCTYPE_DATA_RWEA      0x07    // Data, Read/Write, Expand-Down, Accessed
#define DESCTYPE_CODE_X         0x08    // Code, Execute-Only
#define DESCTYPE_CODE_XA        0x09    // Code, Execute-Only, Accessed
#define DESCTYPE_CODE_XR        0x0A    // Code, Execute/Read
#define DESCTYPE_CODE_XRA       0x0B    // Code, Execute/Read, Accessed
#define DESCTYPE_CODE_XC        0x0C    // Code, Execute-Only, Conforming
#define DESCTYPE_CODE_XCA       0x0D    // Code, Execute-Only, Conforming, Accessed
#define DESCTYPE_CODE_XRC       0x0E    // Code, Execute/Read, Conforming
#define DESCTYPE_CODE_XRCA      0x0F    // Code, Execute/Read, Conforming

/**
 * EFLAGS register bits.
 */
#define EFLAGS_CF           (1 << 0)
#define EFLAGS_PF           (1 << 2)
#define EFLAGS_AF           (1 << 4)
#define EFLAGS_ZF           (1 << 6)
#define EFLAGS_SF           (1 << 7)
#define EFLAGS_TF           (1 << 8)
#define EFLAGS_IF           (1 << 9)
#define EFLAGS_DF           (1 << 10)
#define EFLAGS_OF           (1 << 11)
#define EFLAGS_IOPL         (3 << 12)
#define EFLAGS_NT           (1 << 14)
#define EFLAGS_RF           (1 << 16)
#define EFLAGS_VM           (1 << 17)
#define EFLAGS_AC           (1 << 18)
#define EFLAGS_VIF          (1 << 19)
#define EFLAGS_VIP          (1 << 20)
#define EFLAGS_ID           (1 << 21)

/**
 * CR0 register bits.
 */
#define CR0_PE              (1 << 0)    // Protection Enable
#define CR0_MP              (1 << 1)    // Monitor Coprocessor
#define CR0_EM              (1 << 2)    // x87 Emulation
#define CR0_TS              (1 << 3)    // Task Switched
#define CR0_ET              (1 << 4)    // Extension Type
#define CR0_NE              (1 << 5)    // Numeric Error
#define CR0_WP              (1 << 16)   // Write Protect
#define CR0_AM              (1 << 18)   // Alignment Mask
#define CR0_NW              (1 << 29)   // Non Write-Through
#define CR0_CD              (1 << 30)   // Cache Disable
#define CR0_PG              (1 << 31)   // Paging

/**
 * CR3 register bits.
 */
#define CR3_PWT             (1 << 3)    // Page-Level Write-Through
#define CR3_PCD             (1 << 4)    // Page-Level Cache Disable

/**
 * CR4 register bits.
 */
#define CR4_VME             (1 << 0)    // Virtual-8086 Mode Extensions
#define CR4_PVI             (1 << 1)    // Protected Mode Virtual Interrupts
#define CR4_TSD             (1 << 2)    // Time Stamp Disable
#define CR4_DE              (1 << 3)    // Debugging Extensions
#define CR4_PSE             (1 << 4)    // Page Size Extensions
#define CR4_MCE             (1 << 6)    // Machine Check Enable

/**
 * Page Fault error code bits.
 */
#define PF_P                (1 << 0)
#define PF_WR               (1 << 1)
#define PF_US               (1 << 2)
#define PF_RSVD             (1 << 3)
#define PF_ID               (1 << 4)
#define PF_PK               (1 << 5)
#define PF_SS               (1 << 6)
#define PF_SGX              (1 << 15)

#ifdef __ASSEMBLER__
// Assembler-only defines
.macro LOAD_SEGMENT addr, reg
        movw            \addr, %ax
        movw            %ax, \reg
.endm
.macro STORE_SEGMENT reg, addr
        movw            \reg, %ax
        movw            %ax, \addr
.endm
#else
// C-only defines
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * EFLAGS Register
 */
struct eflags {
    union {
        struct {
            uint32_t cf     : 1;    // Carry Flag
            uint32_t        : 1;    // (reserved, set to 1)
            uint32_t pf     : 1;    // Parity Flag
            uint32_t        : 1;    // (reserved, set to 0)
            uint32_t af     : 1;    // Auxiliary Carry Flag
            uint32_t        : 1;    // (reserved, set to 0)
            uint32_t zf     : 1;    // Zero Flag
            uint32_t sf     : 1;    // Sign Flag
            uint32_t tf     : 1;    // Trap Flag
            uint32_t intf   : 1;    // Interrupt Flag (IF)
            uint32_t df     : 1;    // Direction Flag
            uint32_t of     : 1;    // Overflow Flag
            uint32_t iopl   : 2;    // I/O Privilege Level
            uint32_t nt     : 1;    // Nested Task Flag
            uint32_t        : 1;    // (reserved, set to 0)
            uint32_t rf     : 1;    // Resume Flag
            uint32_t vm     : 1;    // Virtual-8086 Mode
            uint32_t ac     : 1;    // Alignment Check / Access Control
            uint32_t vif    : 1;    // Virtual Interrupt Flag
            uint32_t vip    : 1;    // Virtual Interrupt Pending
            uint32_t id     : 1;    // Identification Flag
            uint32_t        : 10;   // (reserved, set to 0)
        };
        uint32_t _value;
    };
};
static_assert(sizeof(struct eflags) == 4, "sizeof(struct eflags) == 4");

/**
 * x86 Descriptor
 *
 * An x86 Descriptor is a data structure in the GDT, LDT, or IDT that provides
 * the CPU with access control, status, and location/size information about a
 * memory segment, interrupt handler, system task, or program control transfer
 * between different privilege levels.
 *
 * Descriptor Types:
 * - Segment Descriptor: Defines access control, status, location, and size
 *      information for a memory segment or a system segment (such as the LDT).
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
struct x86_desc {
    union {
        struct seg_desc {        // Code/Data Segment Descriptor (GDT/LDT)
            uint64_t limitlo    : 16;   // segment limit[15:0]
            uint64_t baselo     : 24;   // segment base[23:0]
            uint64_t type       : 4;    // segment descriptor type; set to one of DESCTYPE_CODE_* or DESCTYPE_DATA_*
            uint64_t s          : 1;    // system flag; set to 1 for code/data segment
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // segment present in memory
            uint64_t limithi    : 4;    // segment limit[19:16]
            uint64_t avl        : 1;    // (available for software to use)
            uint64_t            : 1;    // reserved; set to 0
            uint64_t db         : 1;    // (Code/Stack) 0 = 16 bit, 1 = 32-bit; (Expand-Down) 0 = 64K, 1 = 4G
            uint64_t g          : 1;    // limit granularity; 0 = byte, 1 = 4K page
            uint64_t basehi     : 8;    // segment base[31:24]
        } seg;
        struct tss_gate {        // TSS Descriptor (GDT)
            uint64_t limitlo    : 16;   // TSS limit[15:0]
            uint64_t baselo     : 24;   // TSS base[bits 23:0]
            uint64_t type       : 4;    // descriptor type; set to one of DESCTYPE_TSS*
            uint64_t            : 1;    // reserved; set to 0
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // segment present in memory
            uint64_t limithi    : 4;    // TSS limit[19:16]
            uint64_t avl        : 1;    // (available for software to use)
            uint64_t            : 1;    // reserved; set to 0
            uint64_t            : 1;    // reserved; set to 0
            uint64_t g          : 1;    // limit granularity; 0 = byte, 1 = 4K page
            uint64_t basehi     : 8;    // TSS base[31:24]
        } tss;
        struct task_gate {       // Task Gate Descriptor (GDT/LDT/IDT)
            uint64_t            : 16;   // reserved; set to 0
            uint64_t segsel     : 16;   // TSS segment selector
            uint64_t            : 8;    // reserved; set to 0
            uint64_t type       : 4;    // descriptor Type; set to DESCTYPE_TASK
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // segment present in memory
            uint64_t            : 16;   // reserved; set to 0
        } task;
        struct call_gate {       // Call Gate Descriptor (GDT/LDT)
            uint64_t offsetlo   : 16;   // code entrypoint[15:0]
            uint64_t segsel     : 16;   // code segment selector
            uint64_t num_params : 5;    // number of stack parameters
            uint64_t            : 3;    // reserved; set to 0
            uint64_t type       : 4;    // descriptor type; set to one of DESCTYPE_CALL*
            uint64_t            : 1;    // reserved; set to 0
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // segment present in memory
            uint64_t offsethi   : 16;   // code entrypoint[31:16]
        } call;
        struct intr_gate {       // Interrupt Gate Descriptor (IDT)
            uint64_t offsetlo   : 16;   // code entrypoint[15:0]
            uint64_t segsel     : 16;   // code segment selector
            uint64_t            : 8;    // reserved; set to 0
            uint64_t type       : 4;    // descriptor type; set to one of DESCTYPE_INTR*
            uint64_t            : 1;    // reserved; set to 0
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // present in memory
            uint64_t offsethi   : 16;   // code entrypoint[31:16]
        } intr;
        struct trap_gate {       // Trap Gate Descriptor (IDT)
            uint64_t offsetlo   : 16;   // code entrypoint[15:0]
            uint64_t segsel     : 16;   // code segment selector
            uint64_t            : 8;    // reserved; set to 0
            uint64_t type       : 4;    // descriptor type; set to one of DESCTYPE_TRAP*
            uint64_t            : 1;    // reserved; set to 0
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // segment present in memory
            uint64_t offsethi   : 16;   // code entrypoint[31:16]
        } trap;
        uint64_t _value;        // Descriptor bits represented as an integer
    };
};
static_assert(sizeof(struct x86_desc) == DESC_SIZE, "sizeof(struct x86_desc) == DESC_SIZE");

/**
 * Segment Selector
 *
 * A Segment Selector is 16-bit identifier for a segment. It points to the
 * Segment Descriptor that defines the segment (located in the GDT or LDT); it
 * is effectively an index into one of the descriptor tables with some extra
 * information. Segment Selectors are loaded into the segment registers (CS, DS,
 * ES, FS, GS, and SS).
 *
 * See Intel Software Developer's Manual, Volume 3A, section 3.4.2.
 */
struct segsel {
    union {
        struct {
            uint16_t rpl    : 2;    // requested privilege level
            uint16_t ti     : 1;    // table indicator; 0 = GDT, 1 = LDT
            uint16_t index  : 13;   // descriptor table index
        };
        uint16_t _value;            // Segment Selector bits represented as an integer
    };
};
static_assert(sizeof(struct segsel) == SEGSEL_SIZE, "sizeof(struct segsel) == SEGSEL_SIZE");

/**
 * Pseudo-Descriptor
 *
 * A Pseudo-Descriptor represents the data structure supplied in the LGDT and
 * LIDT instructions and stored in the SGDT and SIDT instructions.
 *
 * The manual recommends aligning the "limit" field to an odd word address (that
 * is, address MOD 4 is equal to 2) in order to avoid an alignment check fault.
 *
 * See Intel Software Developer's Manual, Volume 3A, section 7.2.
 */
struct pseudo_desc
{
    uint16_t limit;     // GDT or IDT base address
    uint32_t base;      // GDT or IDT limit
} __pack __align(2);
static_assert(sizeof(struct pseudo_desc) == 6, "sizeof(struct pseudo_desc) == 6");

/**
 * Task State Segment
 *
 * The Task State Segment (TSS) contains processor state information needed to
 * save and restore a task.
 *
 * See Intel Software Developer's Manual, Volume 3A, section 7.2.
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
    uint16_t dbgtrap : 1;
    uint16_t _reserved11 : 15;
    uint16_t iomap_base;
    uint32_t ssp;
};
static_assert(sizeof(struct tss) == TSS_SIZE, "sizeof(struct tss) == TSS_SIZE");

/**
 * Configures a Segment Descriptor as a 32-bit Code or Data Segment. Code/Data
 * Segment descriptors go in the GDT or LDT.
 *
 * @param desc segment descriptor pointer
 * @param dpl segment descriptor privilege level
 * @param base segment base address
 * @param limit segment limit (size - 1)
 * @param type segment type (one of DESCTYPE_MEM_CODE_* or DESCTYPE_MEM_DATA_*)
 *
 */
static inline void make_seg_desc(struct x86_desc *desc, int dpl, int base, int limit, int type)
{
    desc->_value = 0;
    desc->seg.type = type;
    desc->seg.dpl = dpl;
    desc->seg.s = 1;       // 1 = memory descriptor (code/data)
    desc->seg.db = 1;      // 1 = 32-bit
    desc->seg.baselo = ((base) & 0x00FFFFFF);
    desc->seg.basehi = ((base) & 0xFF000000) >> 24;
    desc->seg.limitlo = ((limit) & 0x0FFFF);
    desc->seg.limithi = ((limit) & 0xF0000) >> 16;
    desc->seg.g = 1;       // 1 = 4K page granularity
    desc->seg.p = 1;       // 1 = present in memory
}

/**
 * Configures a System Segment Descriptor as a 32-bit LDT Segment. LDT Segment
 * descriptors go in the GDT.
 *
 * @param desc LDT segment descriptor pointer
 * @param dpl LDT descriptor privilege level
 * @param base LDT base address
 * @param limit LDT limit (size - 1)
 */
static inline void make_ldt_desc(struct x86_desc *desc, int dpl, int base, int limit)
{
    desc->_value = 0;
    desc->seg.type = DESCTYPE_LDT;
    desc->seg.dpl = dpl;
    desc->seg.s = 0;       // 0 = system descriptor
    desc->seg.db = 1;      // 1 = 32-bit
    desc->seg.baselo = ((base) & 0x00FFFFFF);
    desc->seg.basehi = ((base) & 0xFF000000) >> 24;
    desc->seg.limitlo = ((limit) & 0x0FFFF);
    desc->seg.limithi = ((limit) & 0xF0000) >> 16;
    desc->seg.g = 0;       // 0 = byte granularity
    desc->seg.p = 1;       // 1 = present in memory
}

/**
 * Configures a System Segment Descriptor as a 32-bit TSS Descriptor. TSS
 * Segment descriptors go in the GDT.
 *
 * @param desc TSS segment descriptor pointer
 * @param dpl TSS descriptor privilege level
 * @param base TSS base address
 * @param limit TSS limit (size - 1)
 */
static inline void make_tss_desc(struct x86_desc *desc, int dpl, int base, int limit)
{
    desc->_value = 0;
    desc->tss.type = DESCTYPE_TSS32;
    desc->tss.dpl = dpl;
    desc->tss.baselo = ((base) & 0x00FFFFFF);
    desc->tss.basehi = ((base) & 0xFF000000) >> 24;
    desc->tss.limitlo = ((limit) & 0x0FFFF);
    desc->tss.limithi = ((limit) & 0xF0000) >> 16;
    desc->tss.g = 0;    // 0 = byte granularity
    desc->tss.p = 1;    // 1 = present in memory
}

/**
 * Configures a System Segment Descriptor as a Task Gate.
 * A Task Gate descriptor provides an indirect, protected reference to a task. A
 * Task Gate is similar to a Call Gate, except that it provides access (through
 * a segment selector) to a TSS rather than a code segment. Task Gate
 * descriptors go in the IDT.
 *
 * @param desc Task Gate segment descriptor pointer
 * @param tss_segsel TSS Segment Selector
 * @param dpl task gate privilege level
 */
static inline void make_task_gate(struct x86_desc *desc, int tss_segsel, int dpl)
{
    desc->_value = 0;
    desc->task.type = DESCTYPE_TASK;
    desc->task.segsel = tss_segsel;
    desc->task.dpl = dpl;
    desc->task.p = 1;  // 1 = present in memory
}

/**
 * Configures a System Segment Descriptor as a 32-bit Call Gate.
 * Call Gates facilitate controlled transfers of program control between
 * different privilege levels in a non-interrupt context (i.e. using the CALL
 * instruction). Call Gates descriptors go in the LDT.
 *
 * @param desc Call Gate segment descriptor pointer
 * @param segsel call handler code segment selector
 * @param dpl Call Gate descriptor privilege level
 * @param num_params number of stack parameters
 * @param handler a pointer to the call handler function
 */
static inline void make_call_gate(struct x86_desc *desc, int segsel, int dpl, int num_params, void *handler)
{
    desc->_value = 0;
    desc->call.type = DESCTYPE_CALL32;
    desc->call.segsel = segsel;
    desc->call.dpl = dpl;
    desc->call.num_params = num_params;
    desc->call.offsetlo = ((uint32_t) handler) & 0xFFFF;
    desc->call.offsethi = ((uint32_t) handler) >> 16;
    desc->call.p = (handler != NULL);
}

/**
 * Configures a System Segment Descriptor as a 32-bit Interrupt Gate.
 * An Interrupt Gate is like a Call Gate, except it clears [IF] after EFLAGS is
 * pushed, preventing other interrupts from interfering with the current
 * handler. Interrupt Gate descriptors go in the IDT.
 *
 * @param desc Interrupt Gate segment descriptor pointer
 * @param segsel interrupt handler code segment selector
 * @param dpl interrupt Gate descriptor privilege level
 * @param handler a pointer to the interrupt handler function
 */
static inline void make_intr_gate(struct x86_desc *desc, int segsel, int dpl, void *handler)
{
    desc->_value = 0;
    desc->intr.type = DESCTYPE_INTR32;
    desc->intr.segsel = segsel;
    desc->intr.dpl = dpl;
    desc->intr.offsetlo = ((uint32_t) handler) & 0xFFFF;
    desc->intr.offsethi = ((uint32_t) handler) >> 16;
    desc->intr.p = (handler != NULL);
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
static inline void make_trap_gate(struct x86_desc *desc, int segsel, int dpl, void *handler)
{
    desc->_value = 0;
    desc->trap.type = DESCTYPE_TRAP32;
    desc->trap.segsel = segsel;
    desc->trap.dpl = dpl;
    desc->trap.offsetlo = ((uint32_t) handler) & 0xFFFF;
    desc->trap.offsethi = ((uint32_t) handler) >> 16;
    desc->trap.p = (handler != NULL);
}

/**
 * Loads the Global Descriptor Table Register (GDTR).
 *
 * @param desc a pointer to the "pseudo-descriptor" that contains the limit and
 *              base address of the LDT; the alignment on desc structure is
 *              tricky; the limit field should be aligned to an odd-word address
 *              (address MOD 4 equals 2)
 */
#define __lgdt(desc)        \
__asm__ volatile (          \
    "lgdt %0"               \
    :                       \
    : "m"(desc)             \
)

/**
 * Loads the Interrupt Descriptor Table Register (IDTR).
 *
 * @param desc a pointer to the "pseudo-descriptor" that contains the limit and
 *              base address of the IDT; the alignment on desc structure is
 *              tricky; the limit field should be aligned to an odd-word address
 *              (address MOD 4 equals 2)
 */
#define __lidt(desc)        \
__asm__ volatile (          \
    "lidt %0"               \
    :                       \
    : "m"(desc)             \
)

/**
 * Loads the Local Descriptor Table Register (LDTR).
 *
 * @param segsel segment selector for the LDT
 */
#define __lldt(segsel)      \
__asm__ volatile (          \
    "lldt %w0"              \
    :                       \
    : "r"(segsel)           \
)

/**
 * Loads the Task Register (TR).
 *
 * @param segsel segment selector for the TSS
 */
#define __ltr(segsel)       \
__asm__ volatile (          \
    "ltr %w0"               \
    :                       \
    : "r"(segsel)           \
)

/**
 * Clears the interrupt flag, disabling interrupts.
 */
#define __cli()                                                             \
__asm__ volatile (                                                          \
    "cli"                                                                   \
    :                                                                       \
    :                                                                       \
    : "cc"                                                                  \
)

/**
 * Sets the interrupt flag, enabling interrupts.
 */
#define __sti()                                                             \
__asm__ volatile (                                                          \
    "sti"                                                                   \
    :                                                                       \
    :                                                                       \
    : "cc"                                                                  \
)

/**
 * Executes the CPUID instruction.
 *
 * @param eax CPUID command
 * @param param0 EAX returned by CPUID
 * @param param1 EBX returned by CPUID
 * @param param2 ECX returned by CPUID
 * @param param3 EDX returned by CPUID
 */
#define __cpuid(eax,param0,param1,param2,param3)                            \
__asm__ volatile (                                                          \
    "cpuid"                                                                 \
    : "=a"(param0), "=b"(param1), "=c"(param2), "=d"(param3)                \
    : "a"(eax)                                                              \
)

#define  read_cs(cs) __asm__ volatile ("movw %%cs, %%ax"      : "=a"(cs):)
#define  read_ds(cs) __asm__ volatile ("movw %%ds, %%ax"      : "=a"(ds):)
#define  read_es(cs) __asm__ volatile ("movw %%es, %%ax"      : "=a"(es):)
#define  read_fs(cs) __asm__ volatile ("movw %%fs, %%ax"      : "=a"(fs):)
#define  read_gs(cs) __asm__ volatile ("movw %%gs, %%ax"      : "=a"(gs):)
#define  read_ss(cs) __asm__ volatile ("movw %%ss, %%ax"      : "=a"(ss):)
#define write_cs(cs) __asm__ volatile ("ljmpl %0, $x%=; x%=:" : : "I"(cs))
#define write_ds(ds) __asm__ volatile ("movw %%ax, %%ds"      : : "a"(ds))
#define write_es(es) __asm__ volatile ("movw %%ax, %%es"      : : "a"(es))
#define write_fs(fs) __asm__ volatile ("movw %%ax, %%fs"      : : "a"(fs))
#define write_gs(gs) __asm__ volatile ("movw %%ax, %%gs"      : : "a"(gs))
#define write_ss(ss) __asm__ volatile ("movw %%ax, %%ss"      : : "a"(ss))

#define  read_eax(eax) __asm__ volatile ("" : "=a"(eax):)
#define  read_ebx(ebx) __asm__ volatile ("" : "=b"(ebx):)
#define  read_ecx(ecx) __asm__ volatile ("" : "=c"(ecx):)
#define  read_edx(edx) __asm__ volatile ("" : "=d"(edx):)
#define  read_esi(esi) __asm__ volatile ("" : "=S"(esi):)
#define  read_edi(edi) __asm__ volatile ("" : "=D"(edi):)
#define write_eax(eax) __asm__ volatile ("" : : "a"(eax))
#define write_ebx(ebx) __asm__ volatile ("" : : "b"(ebx))
#define write_ecx(ecx) __asm__ volatile ("" : : "c"(ecx))
#define write_edx(edx) __asm__ volatile ("" : : "d"(edx))
#define write_esi(esi) __asm__ volatile ("" : : "S"(esi))
#define write_edi(edi) __asm__ volatile ("" : : "D"(edi))

#define  read_cr0(cr0) __asm__ volatile ("movl %%cr0, %%eax" : "=a"(cr0):)
#define  read_cr2(cr2) __asm__ volatile ("movl %%cr2, %%eax" : "=a"(cr2):)
#define  read_cr3(cr3) __asm__ volatile ("movl %%cr3, %%eax" : "=a"(cr3):)
#define  read_cr4(cr4) __asm__ volatile ("movl %%cr4, %%eax" : "=a"(cr4):)
#define write_cr0(cr0) __asm__ volatile ("movl %%eax, %%cr0" : : "a"(cr0))
#define write_cr2(cr2) __asm__ volatile ("movl %%eax, %%cr2" : : "a"(cr2))
#define write_cr3(cr3) __asm__ volatile ("movl %%eax, %%cr3" : : "a"(cr3))
#define write_cr4(cr4) __asm__ volatile ("movl %%eax, %%cr4" : : "a"(cr4))

/**
 * Page Directory Entry for 32-bit Paging
 *
 * Points to a 4M page or a 4K page table.
 */
struct pde
{
    uint32_t p      : 1;    // Present
    uint32_t rw     : 1;    // Read/Write; 1 = writable
    uint32_t us     : 1;    // User/Supervisor; 1 = user accessible
    uint32_t pwt    : 1;    // Page-Level Write-Through
    uint32_t pcd    : 1;    // Page-Level Cache Disable
    uint32_t a      : 1;    // Accessed; software has accessed this page
    uint32_t d      : 1;    // Dirty; software has written this page
    uint32_t ps     : 1;    // Page Size; 0 = 4K page table, 1 = 4M page
    uint32_t g      : 1;    // Global; pins page to TLB (requires CR4.PGE=1)
    uint32_t        : 3;    // (reserved)
    uint32_t address: 20;   // Address of 4M page or 4K page table
};
static_assert(sizeof(struct pde) == 4, "bad PDE size!");

/**
 * Page Table Entry for 32-bit Paging
 *
 * Points to a 4K page.
 */
struct pte
{
    uint32_t p      : 1;    // Present
    uint32_t rw     : 1;    // Read/Write; 1 = writable
    uint32_t us     : 1;    // User/Supervisor; 1 = user accessible
    uint32_t pwt    : 1;    // Page-Level Write-Through
    uint32_t pcd    : 1;    // Page-Level Cache Disable
    uint32_t a      : 1;    // Accessed; software has accessed this page
    uint32_t d      : 1;    // Dirty; software has written this page
    uint32_t        : 1;    // (reserved, PAT)
    uint32_t g      : 1;    // Global; pins page to TLB (requires CR4.PGE=1)
    uint32_t        : 3;    // (reserved)
    uint32_t address: 20;   // Address of 4K page
};
static_assert(sizeof(struct pte) == 4, "bad PTE size!");

#endif /* __ASSEMBLER__ */

#endif /* __X86_H */
