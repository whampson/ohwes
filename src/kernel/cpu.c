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
 *         File: kernel/cpu.c
 *      Created: December 24, 2023
 *       Author: Wes Hampson
 *
 * Intel x86 CPU initialization for 32-bit Protected Mode.
 * =============================================================================
 */

#include <boot.h>
#include <cpu.h>
#include <interrupt.h>
#include <irq.h>

#define CPU_DATA_AREA       0x1000
#define PAGE_2              0x2000

//
// x86 Descriptor table and TSS geometry.
//
// IDT                                              // 0x800
#define IDT_COUNT           256
#define IDT_BASE            CPU_DATA_AREA
#define IDT_LIMIT           (IDT_COUNT*DESC_SIZE-1)
#define IDT_SIZE            (IDT_LIMIT+1)
// GDT                                              //
#define GDT_COUNT           8                       //
#define GDT_BASE            (IDT_BASE+IDT_SIZE)     // GDT immediately follows IDT
#define GDT_LIMIT           (GDT_COUNT*DESC_SIZE-1)
#define GDT_SIZE            (GDT_LIMIT+1)
// LDT
#define LDT_COUNT           2
#define LDT_BASE            (GDT_BASE+GDT_SIZE)     // LDT immediately follows GDT
#define LDT_LIMIT           (LDT_COUNT*DESC_SIZE-1)
#define LDT_SIZE            (LDT_LIMIT+1)
// TSS
#define TSS_BASE            (LDT_BASE+LDT_SIZE)     // TSS immediately follows LDT
#define TSS_LIMIT           (TSS_SIZE-1)            // TSS_SIZE is fixed, defined in x86.h

static_assert(IDT_BASE + IDT_LIMIT < GDT_BASE, "IDT overlaps GDT!");
static_assert(GDT_BASE + GDT_LIMIT < LDT_BASE, "GDT overlaps LDT!");
static_assert(LDT_BASE + LDT_LIMIT < TSS_BASE, "LDT overlaps TSS!");
static_assert(TSS_BASE + TSS_LIMIT <= PAGE_2, "TSS overlaps into next page!");


//
// GDT Segment Selectors
//
#define SEGSEL_NULL         (0x0)
#define SEGSEL_LDT          (0x08|KERNEL_PL)
#define SEGSEL_KERNEL_CODE  (0x10|KERNEL_PL)
#define SEGSEL_KERNEL_DATA  (0x18|KERNEL_PL)
#define SEGSEL_USER_CODE    (0x20|USER_PL)
#define SEGSEL_USER_DATA    (0x28|USER_PL)
#define SEGSEL_TSS          (0x30|KERNEL_PL)

static_assert(KERNEL_CS == SEGSEL_KERNEL_CODE, "KERNEL_CS invalid!");
static_assert(KERNEL_DS == SEGSEL_KERNEL_DATA, "KERNEL_DS invalid!");
static_assert(USER_CS == SEGSEL_USER_CODE, "USER_CS invalid!");
static_assert(USER_DS == SEGSEL_USER_DATA, "USER_DS invalid!");

/**
 * Gets a Segment Descriptor from a descriptor table.
 *
 * @param table a pointer to the descriptor table
 * @param segsel the segment selector used to index the table
 * @return a pointer to the segment descriptor specified by the segment selector
 */
#define get_desc(table,segsel) \
    (&((struct x86_desc*)(table))[(segsel)>>3])

struct tss * get_tss(struct tss *tss)
{
    return (struct tss *) TSS_BASE;
}

struct x86_desc * get_seg_desc(uint16_t segsel)
{
    struct segsel *ss = (struct segsel *) &segsel;

    if (ss->ti) {
        if (segsel > LDT_LIMIT) {
            panic("LDT segment selector out of range - %04x", segsel);
        }
        return get_desc(LDT_BASE, segsel);
    }

    if (segsel > GDT_LIMIT) {
        panic("GDT segment selector out of range - %04x", segsel);
    }
    return get_desc(GDT_BASE, segsel);
}

static void init_gdt(const struct bootinfo * const info);
static void init_idt(const struct bootinfo * const info);
static void init_ldt(const struct bootinfo * const info);
static void init_tss(const struct bootinfo * const info);

void init_cpu(const struct bootinfo * const info)
{
    init_gdt(info);
    init_idt(info);
    init_ldt(info);
    init_tss(info);
}

static void init_gdt(const struct bootinfo * const info)
{
    //
    // Global Descriptor Table (GDT) Initialization
    //
    // See Intel Software Developer's Manual, Vol 3A
    //     sections 3.4 & 3.5
    //

    // Zero GDT
    memset((void *) GDT_BASE, 0, GDT_SIZE);

    // Open up all of memory (0-4G) for use by kernel and user
    struct x86_desc *kernel_cs = get_desc(GDT_BASE, SEGSEL_KERNEL_CODE);
    struct x86_desc *kernel_ds = get_desc(GDT_BASE, SEGSEL_KERNEL_DATA);
    struct x86_desc *user_cs = get_desc(GDT_BASE, SEGSEL_USER_CODE);
    struct x86_desc *user_ds = get_desc(GDT_BASE, SEGSEL_USER_DATA);

    make_seg_desc(kernel_cs, KERNEL_PL, 0x0, LIMIT_MAX, DESCTYPE_CODE_XR);  // KERNEL_CS -> DPL0, Execute/Read
    make_seg_desc(kernel_ds, KERNEL_PL, 0x0, LIMIT_MAX, DESCTYPE_DATA_RW);  // KERNEL_DS -> DPL0, Read/Write
    make_seg_desc(user_cs,   USER_PL,   0x0, LIMIT_MAX, DESCTYPE_CODE_XR);  // USER_CS   -> DPL3, Execute/Read
    make_seg_desc(user_ds,   USER_PL,   0x0, LIMIT_MAX, DESCTYPE_DATA_RW);  // USER_DS   -> DPL3, Read/Write

    // Create GDT descriptor and use it to load GDTR, effectively "setting" the GDT
    struct pseudo_desc gdt_desc = { .base = GDT_BASE, .limit = GDT_LIMIT };
    lgdt(gdt_desc);

    // Reload segment registers with new segment selectors
    load_cs(SEGSEL_KERNEL_CODE);
    load_ds(SEGSEL_KERNEL_DATA);
    load_es(SEGSEL_KERNEL_DATA);
    load_fs(SEGSEL_NULL);
    load_gs(SEGSEL_NULL);
    load_ss(SEGSEL_KERNEL_DATA);
}

static void init_idt(const struct bootinfo * const info)
{
    static const idt_thunk excepts[NUM_EXCEPTION] =
    {
        _thunk_except00h, _thunk_except01h, _thunk_except02h, _thunk_except03h,
        _thunk_except04h, _thunk_except05h, _thunk_except06h, _thunk_except07h,
        _thunk_except08h, _thunk_except09h, _thunk_except0Ah, _thunk_except0Bh,
        _thunk_except0Ch, _thunk_except0Dh, _thunk_except0Eh, _thunk_except0Fh,
        _thunk_except10h, _thunk_except11h, _thunk_except12h, _thunk_except13h,
        _thunk_except14h, _thunk_except15h, _thunk_except16h, _thunk_except17h,
        _thunk_except18h, _thunk_except19h, _thunk_except1Ah, _thunk_except19h,
        _thunk_except1Ch, _thunk_except1Ch, _thunk_except1Eh, _thunk_except1Fh
    };

    static const idt_thunk irqs[NUM_IRQ] =
    {
        _thunk_irq00h, _thunk_irq01h, _thunk_irq02h, _thunk_irq03h,
        _thunk_irq04h, _thunk_irq05h, _thunk_irq06h, _thunk_irq07h,
        _thunk_irq08h, _thunk_irq09h, _thunk_irq0Ah, _thunk_irq0Bh,
        _thunk_irq0Ch, _thunk_irq0Dh, _thunk_irq0Eh, _thunk_irq0Fh
    };

    // Zero IDT
    memset((void *) IDT_BASE, 0, IDT_SIZE);

    // Fill IDT
    int count = IDT_SIZE / sizeof(struct x86_desc);
    for (int idx = 0, e = 0, i = 0; idx < count; idx++) {
        struct x86_desc *desc = ((struct x86_desc *) IDT_BASE) + idx;
        e = idx - IVT_EXCEPTION;
        i = idx - IVT_DEVICEIRQ;

        if (idx >= IVT_EXCEPTION && e < NUM_EXCEPTION) {
            // interrupt gate for exceptions;
            // probably a good idea to handle exceptions with no interruptions
            make_intr_gate(desc, SEGSEL_KERNEL_CODE, KERNEL_PL, excepts[e]);
        }
        else if (idx >= IVT_DEVICEIRQ && i < NUM_IRQ) {
            // interrupt gate for device IRQs;
            // we don't want other devices interrupting handler!
            make_intr_gate(desc, SEGSEL_KERNEL_CODE, KERNEL_PL, irqs[i]);
        }
        else if (idx == IVT_SYSCALL) {
            // user-mode accessible trap gate for system calls;
            // devices can interrupt system call
            make_trap_gate(desc, SEGSEL_KERNEL_CODE, USER_PL, _thunk_syscall);
        }
        // else, keep the vector NULL! will generate a double-fault exception
    }

    // Load the IDTR
    struct pseudo_desc idt_desc = { .base = IDT_BASE, .limit = IDT_LIMIT };
    lidt(idt_desc);
}

static void init_ldt(const struct bootinfo * const info)
{
    //
    // LDT loaded but not used
    //

    // Zero LDT
    memset((void *) LDT_BASE, 0, IDT_SIZE);

    // Create LDT entry in GDT using predefined LDT segment selector.
    void *ldt_desc = get_desc(GDT_BASE, SEGSEL_LDT);
    make_ldt_desc(ldt_desc, KERNEL_PL, LDT_BASE, LDT_LIMIT);

    // Load LDTR
    lldt(SEGSEL_LDT);
}

static void init_tss(const struct bootinfo * const info)
{
    //
    // TSS used minimally; only when an interrupt occurs that changes CPU to
    // privilege level 0 (kernel mode) in order to locate the kernel stack.
    //

    // Zero TSS
    struct tss * const tss = (struct tss *) TSS_BASE;
    memset(tss, 0, TSS_SIZE);

    // Fill TSS with LDT segment selector, kernel stack pointer,
    // and kernel stack segment selector
    tss->ldt_segsel = SEGSEL_LDT;
    tss->esp0 = (uint32_t) info->stack_base;     // TODO: make this WELL-DEFINED!!
    tss->ss0 = SEGSEL_KERNEL_DATA;

    // Add TSS entry to GDT
    void *pTssDesc = get_desc(GDT_BASE, SEGSEL_TSS);
    make_tss_desc(pTssDesc, KERNEL_PL, TSS_BASE, TSS_LIMIT);

    // Load Task Register
    ltr(SEGSEL_TSS);
}
