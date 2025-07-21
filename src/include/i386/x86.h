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
 * EFLAGS Register Fields
 */
#define EFLAGS_CF           (1 << 0)    // Carry Flag
#define EFLAGS_PF           (1 << 2)    // Parity Flag
#define EFLAGS_AF           (1 << 4)    // Auxiliary Carry Flag
#define EFLAGS_ZF           (1 << 6)    // Zero Flag
#define EFLAGS_SF           (1 << 7)    // Sign Flag
#define EFLAGS_TF           (1 << 8)    // Trap Flag
#define EFLAGS_IF           (1 << 9)    // Interrupt Flag
#define EFLAGS_DF           (1 << 10)   // Direction Flag
#define EFLAGS_OF           (1 << 11)   // Overflow Flag
#define EFLAGS_IOPL         (3 << 12)   // I/O Privilege Level
#define EFLAGS_NT           (1 << 14)   // Nested Task Flag
#define EFLAGS_RF           (1 << 16)   // Resume Flag
#define EFLAGS_VM           (1 << 17)   // Virtual 8086 Mode
#define EFLAGS_AC           (1 << 18)   // Alignment Check (486+)
#define EFLAGS_VIF          (1 << 19)   // Virtual Interrupt Flag (Pentium+)
#define EFLAGS_VIP          (1 << 20)   // Virtual Interrupt Pending (Pentium+)
#define EFLAGS_ID           (1 << 21)   // CPUID Instruction Present (Pentium+)

/**
 * CR0 Register Fields
 */
#define CR0_PE              (1 << 0)    // Protected Mode Enable
#define CR0_MP              (1 << 1)    // Monitor Coprocessor
#define CR0_EM              (1 << 2)    // x87 Emulation
#define CR0_TS              (1 << 3)    // Task Switched
#define CR0_ET              (1 << 4)    // Extension Type (80287 or 80387)
#define CR0_NE              (1 << 5)    // Numeric Error (x86 FPU error)
#define CR0_WP              (1 << 16)   // Write Protect
#define CR0_AM              (1 << 18)   // Alignment Mask
#define CR0_NW              (1 << 29)   // Non Write-Through
#define CR0_CD              (1 << 30)   // Cache Disable
#define CR0_PG              (1 << 31)   // Paging Enable

/**
 * CR3 Register Fields
 */
#define CR3_PWT             (1 << 3)    // Page-Level Write-Through
#define CR3_PCD             (1 << 4)    // Page-Level Cache Disable
#define CR3_PGDIR           (0xFFFFF << 12) // Page Directory Base

/**
 * CR4 Register Fields
 */
#define CR4_VME             (1 << 0)    // Virtual-8086 Mode Extensions
#define CR4_PVI             (1 << 1)    // Protected Mode Virtual Interrupts
#define CR4_TSD             (1 << 2)    // Time Stamp Disable
#define CR4_DE              (1 << 3)    // Debugging Extensions
#define CR4_PSE             (1 << 4)    // Page Size Extensions
#define CR4_MCE             (1 << 6)    // Machine Check Enable

/**
 * Error Code Fields
 */
#define ERR_EXT             (1 << 0)    // External event caused exception
#define ERR_IDT             (1 << 1)    // IDT Index
#define ERR_TI              (1 << 2)    // GDT/LDT Index (1=LDT)
#define ERR_INDEX           (0x1FFF<<3) // Segment Selector Index

/**
 * Page Fault Error Code Fields
 */
#define PF_P                (1 << 0)    // Protection Fault (0=Non-Present Page)
#define PF_WR               (1 << 1)    // Read/Write Fault (1=Write)
#define PF_US               (1 << 2)    // User/Supervisor Fault (1=User)
#define PF_RSVD             (1 << 3)    // Reserved Bit Violation

/**
 * TSS Field Offsets
 */

#define TSS_PREV            0x00
#define TSS_ESP0            0x04
#define TSS_SS0             0x08
#define TSS_ESP1            0x0C
#define TSS_SS1             0x10
#define TSS_ESP2            0x14
#define TSS_SS2             0x18
#define TSS_CR3             0x1C
#define TSS_EIP             0x20
#define TSS_EFLAGS          0x24
#define TSS_EAX             0x28
#define TSS_ECX             0x2C
#define TSS_EDX             0x30
#define TSS_EBX             0x34
#define TSS_ESP             0x38
#define TSS_EBP             0x3C
#define TSS_ESI             0x40
#define TSS_EDI             0x44
#define TSS_ES              0x48
#define TSS_CS              0x4C
#define TSS_SS              0x50
#define TSS_DS              0x54
#define TSS_FS              0x58
#define TSS_GS              0x5C
#define TSS_LDTSEG          0x60
#define TSS_DBGTRAP         0x64
#define TSS_IOBASE          0x68
#define TSS_SSP             0x00

#ifdef __ASSEMBLER__    // Assembler-only defines

/**
 * Loads a segment register with the bottom 16 bits of a 32-bit value from
 * memory. Clobbers AX.
 */
.macro LOAD_SEGMENT addr, reg
        movl            \addr, %eax
        movw            %ax, \reg
.endm

/**
 * Stores a segment register in the bottom 16 bits of a 32-bit value in memory.
 * Clobbers EAX.
 */
.macro STORE_SEGMENT reg, addr
        xorl            %eax, %eax
        movw            \reg, %ax
        movl            %eax, \addr
.endm

/**
 * Performs a 32-bit memory-to-memory move.
 * Clobbers EAX.
 */
.macro MEM2MEM src, dest
    movl    \src, %eax
    movl    %eax, \dest
.endm

#else       // C-only defines

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
            uint64_t            : 1;    // system flag; set to 0
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
            uint64_t            : 1;    // reserved; set to 0
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // gate present in memory
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
            uint64_t p          : 1;    // gate present in memory
            uint64_t offsethi   : 16;   // code entrypoint[31:16]
        } call;
        struct intr_gate {       // Interrupt Gate Descriptor (IDT)
            uint64_t offsetlo   : 16;   // code entrypoint[15:0]
            uint64_t segsel     : 16;   // code segment selector
            uint64_t            : 8;    // reserved; set to 0
            uint64_t type       : 4;    // descriptor type; set to one of DESCTYPE_INTR*
            uint64_t            : 1;    // reserved; set to 0
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // gate present in memory
            uint64_t offsethi   : 16;   // code entrypoint[31:16]
        } intr;
        struct trap_gate {       // Trap Gate Descriptor (IDT)
            uint64_t offsetlo   : 16;   // code entrypoint[15:0]
            uint64_t segsel     : 16;   // code segment selector
            uint64_t            : 8;    // reserved; set to 0
            uint64_t type       : 4;    // descriptor type; set to one of DESCTYPE_TRAP*
            uint64_t            : 1;    // reserved; set to 0
            uint64_t dpl        : 2;    // descriptor privilege level
            uint64_t p          : 1;    // gate present in memory
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
 * Table Descriptor
 *
 * A Table Descriptor represents the data structure supplied in the LGDT and
 * LIDT instructions and stored in the SGDT and SIDT instructions used to
 * specify the base and limit of a descriptor table.
 *
 * See Intel Software Developer's Manual, Volume 3A, section 7.2.
 */
struct table_desc
{
    uint16_t limit;     // GDT or IDT base address
    uint32_t base;      // GDT or IDT limit
} __pack __align(2);    // pack to ensure limit and base are contiguous 48 bits
static_assert(sizeof(struct table_desc) == 6, "sizeof(struct table_desc) == 6");

/**
 * Gets a Segment Descriptor from a descriptor table.
 *
 * @param table a pointer to the descriptor table (GDT, LDT, or IDT)
 * @param segsel the segment selector used to index the table
 * @return a pointer to the segment descriptor specified by the segment selector
 */
#define x86_get_desc(table,segsel) \
    (&(((struct x86_desc *)(table))[(segsel&0xFFFF)>>3]))

/**
 * Gets the base address from a Segment Descriptor.
 *
 * @param desc a segment descriptor
 * @return the segment base address
 */
#define x86_seg_base(desc) \
    (((desc)->seg.basehi << 24) | (desc)->seg.baselo)

/**
 * Gets the limit value from a Segment Descriptor.
 *
 * @param desc a segment descriptor
 * @return the segment limit
 */
#define x86_seg_limit(desc) \
    (((desc)->seg.limithi << 16) | (desc)->seg.limitlo)

/**
 * Checks whether a Segment Descriptor exists within a descriptor table.
 *
 * @param table_desc a Table Descriptor containing the descriptor table address
 *                   and limit
 * @param desc a segment descriptor
 * @return 'true' if the segment descriptor exists within the descriptor table
 */
#define x86_desc_valid(table_desc,desc) \
    (((((uint32_t) (desc)) + (sizeof(struct x86_desc) - 1)) & ~(sizeof(struct x86_desc) - 1)) && \
    ((uint32_t) (desc) >= (table_desc).base) && ((uint32_t) (desc) < (table_desc).base + (table_desc).limit))

/**
 * Task State Segment
 *
 * The Task State Segment (TSS) contains processor state information needed to
 * save and restore a task.
 *
 * See Intel Software Developer's Manual, Volume 3A, section 7.2.
 */
struct tss {
    uint32_t prev   : 16;
    uint32_t        : 16;
    uint32_t esp0;
    uint32_t ss0    : 16;
    uint32_t        : 16;
    uint32_t esp1;
    uint32_t ss1    : 16;
    uint32_t        : 16;
    uint32_t esp2;
    uint32_t ss2    : 16;
    uint32_t        : 16;
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
    uint32_t es     : 16;
    uint32_t        : 16;
    uint32_t cs     : 16;
    uint32_t        : 16;
    uint32_t ss     : 16;
    uint32_t        : 16;
    uint32_t ds     : 16;
    uint32_t        : 16;
    uint32_t fs     : 16;
    uint32_t        : 16;
    uint32_t gs     : 16;
    uint32_t        : 16;
    uint32_t ldtseg : 16;
    uint32_t        : 16;
    uint32_t dbgtrap: 1;
    uint32_t        : 15;
    uint32_t iobase;
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
 * @param type segment type (one of DESCTYPE_CODE_* or DESCTYPE_DATA_*)
 *
 */
void make_seg_desc(struct x86_desc *desc, int dpl, int base, int limit, int type);

/**
 * Configures a System Segment Descriptor as a 32-bit LDT Segment. LDT Segment
 * descriptors go in the GDT.
 *
 * @param desc LDT segment descriptor pointer
 * @param dpl LDT descriptor privilege level
 * @param base LDT base address
 * @param limit LDT limit (size - 1)
 */
void make_ldt_desc(struct x86_desc *desc, int dpl, int base, int limit);


/**
 * Configures a System Segment Descriptor as a 32-bit TSS Descriptor. TSS
 * Segment descriptors go in the GDT.
 *
 * @param desc TSS segment descriptor pointer
 * @param dpl TSS descriptor privilege level
 * @param base TSS base address
 */
void make_tss_desc(struct x86_desc *desc, int dpl, struct tss *base);

/**
 * Configures a System Segment Descriptor as a Task Gate.
 * A Task Gate descriptor provides an indirect, protected reference to a task. A
 * Task Gate is similar to a Call Gate, except that it provides access (through
 * a segment selector) to a TSS rather than a code segment. Task Gate
 * descriptors go in the GDT, LDT, or IDT.
 *
 * @param desc Task Gate segment descriptor pointer
 * @param tss_segsel TSS Segment Selector
 * @param dpl task gate privilege level
 */
void make_task_gate(struct x86_desc *desc, int tss_segsel, int dpl);


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
void make_call_gate(struct x86_desc *desc, int segsel, int dpl, int num_params, void *handler);

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
void make_intr_gate(struct x86_desc *desc, int segsel, int dpl, void *handler);


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
void make_trap_gate(struct x86_desc *desc, int segsel, int dpl, void *handler);

#define __cpuid(fn,eax,ebx,ecx,edx)             \
__asm__ volatile (                              \
    "cpuid"                                     \
    : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)\
    :"a"(fn)                                    \
)

#define __cli() __asm__ volatile ("cli")
#define __sti() __asm__ volatile ("sti")

#define __int3()  __asm__ volatile ("int3")

#define __lgdt(table_desc) __asm__ volatile ("lgdt %0" :: "m"(table_desc) : "memory")
#define __sgdt(table_desc) __asm__ volatile ("sgdt %0" :: "m"(table_desc) : "memory")
#define __lidt(table_desc) __asm__ volatile ("lidt %0" :: "m"(table_desc) : "memory")
#define __sidt(table_desc) __asm__ volatile ("sidt %0" :: "m"(table_desc) : "memory")

#define __lldt(segsel) __asm__ volatile ("lldt %w0"  :: "r"(segsel))
#define __sldt(segsel) __asm__ volatile ("sldt %w0"  : "=r"(segsel))
#define __ltr(segsel)  __asm__ volatile ("ltr %w0"   :: "r"(segsel))
#define __str(segsel)  __asm__ volatile ("str %w0"   : "=r"(segsel))

#define  load_cs(cs) __asm__ volatile ("ljmpl %0, $x%=; x%=:" :: "I"(cs))
#define  load_ds(ds) __asm__ volatile ("movw %%ax, %%ds"      :: "a"(ds))
#define  load_es(es) __asm__ volatile ("movw %%ax, %%es"      :: "a"(es))
#define  load_fs(fs) __asm__ volatile ("movw %%ax, %%fs"      :: "a"(fs))
#define  load_gs(gs) __asm__ volatile ("movw %%ax, %%gs"      :: "a"(gs))
#define  load_ss(ss) __asm__ volatile ("movw %%ax, %%ss"      :: "a"(ss))
#define store_cs(cs) __asm__ volatile ("movw %%cs, %%ax"      : "=a"(cs))
#define store_ds(ds) __asm__ volatile ("movw %%ds, %%ax"      : "=a"(ds))
#define store_es(es) __asm__ volatile ("movw %%es, %%ax"      : "=a"(es))
#define store_fs(fs) __asm__ volatile ("movw %%fs, %%ax"      : "=a"(fs))
#define store_gs(gs) __asm__ volatile ("movw %%gs, %%ax"      : "=a"(gs))
#define store_ss(ss) __asm__ volatile ("movw %%ss, %%ax"      : "=a"(ss))

#define  load_eax(eax) __asm__ volatile ("" :: "a"(eax))
#define  load_ebx(ebx) __asm__ volatile ("" :: "b"(ebx))
#define  load_ecx(ecx) __asm__ volatile ("" :: "c"(ecx))
#define  load_edx(edx) __asm__ volatile ("" :: "d"(edx))
#define  load_esi(esi) __asm__ volatile ("" :: "S"(esi))
#define  load_edi(edi) __asm__ volatile ("" :: "D"(edi))
#define store_eax(eax) __asm__ volatile ("" : "=a"(eax):)
#define store_ebx(ebx) __asm__ volatile ("" : "=b"(ebx):)
#define store_ecx(ecx) __asm__ volatile ("" : "=c"(ecx):)
#define store_edx(edx) __asm__ volatile ("" : "=d"(edx):)
#define store_esi(esi) __asm__ volatile ("" : "=S"(esi):)
#define store_edi(edi) __asm__ volatile ("" : "=D"(edi):)

#define  load_cr0(cr0) __asm__ volatile ("movl %%eax, %%cr0" :: "a"(cr0))
#define  load_cr2(cr2) __asm__ volatile ("movl %%eax, %%cr2" :: "a"(cr2))
#define  load_cr3(cr3) __asm__ volatile ("movl %%eax, %%cr3" :: "a"(cr3))
#define  load_cr4(cr4) __asm__ volatile ("movl %%eax, %%cr4" :: "a"(cr4))
#define store_cr0(cr0) __asm__ volatile ("movl %%cr0, %%eax" : "=a"(cr0))
#define store_cr2(cr2) __asm__ volatile ("movl %%cr2, %%eax" : "=a"(cr2))
#define store_cr3(cr3) __asm__ volatile ("movl %%cr3, %%eax" : "=a"(cr3))
#define store_cr4(cr4) __asm__ volatile ("movl %%cr4, %%eax" : "=a"(cr4))

#define flush_tlb()                         \
__asm__ volatile (                          \
    "movl %%cr3, %%eax; movl %%eax, %%cr3"  \
    ::: "eax"                               \
);

/**
 * Page Directory Entry for 32-bit Paging
 *
 * Points to a 4M page or a 4K page table.
 */
struct x86_pde {
    uint32_t p      : 1;    // Present
    uint32_t rw     : 1;    // Read/Write; 1 = writable
    uint32_t us     : 1;    // User/Supervisor; 1 = user accessible
    uint32_t pwt    : 1;    // Page-Level Write-Through
    uint32_t pcd    : 1;    // Page-Level Cache Disable
    uint32_t a      : 1;    // Accessed; software has accessed this page
    uint32_t d      : 1;    // Dirty; software has written this page
    uint32_t ps     : 1;    // Page Size; 0 = 4K page table, 1 = 4M page (requires CR4.PSE=1)
    uint32_t g      : 1;    // Global; pins page to TLB (requires CR4.PGE=1)
    uint32_t        : 3;    // (available for software use)
    uint32_t pfn    : 20;   // Page Frame Number: aligned address of 4K page table or 4M page
};
static_assert(sizeof(struct x86_pde) == 4, "bad PDE size!");

/**
 * Page Table Entry for 32-bit Paging
 *
 * Points to a 4K page.
 */
struct x86_pte {
    uint32_t p      : 1;    // Present
    uint32_t rw     : 1;    // Read/Write; 1 = writable
    uint32_t us     : 1;    // User/Supervisor; 1 = user accessible
    uint32_t pwt    : 1;    // Page-Level Write-Through
    uint32_t pcd    : 1;    // Page-Level Cache Disable
    uint32_t a      : 1;    // Accessed; software has accessed this page
    uint32_t d      : 1;    // Dirty; software has written this page
    uint32_t        : 1;    // (reserved; PAT)
    uint32_t g      : 1;    // Global; pins page to TLB (requires CR4.PGE=1)
    uint32_t        : 3;    // (available for software use)
    uint32_t pfn    : 20;   // Page Frame Number: 4K-aligned address of 4K page
};
static_assert(sizeof(struct x86_pte) == 4, "bad PTE size!");

#endif /* __ASSEMBLER__ */

#endif /* __X86_H */
