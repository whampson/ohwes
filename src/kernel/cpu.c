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
 *         File: kernel/cpu.c
 *      Created: December 24, 2023
 *       Author: Wes Hampson
 *
 * Intel x86 CPU initialization for 32-bit Protected Mode.
 * =============================================================================
 */

#include <boot.h>
#include <compiler.h>
#include <interrupt.h>
#include <x86.h>

//
// x86 Descriptor table and TSS geometry.
//

#define IDT_COUNT           256
#define IDT_BASE            0x1000
#define IDT_LIMIT           (IDT_COUNT*DESC_SIZE-1)
#define IDT_SIZE            (IDT_LIMIT+1)

#define GDT_COUNT           8
#define GDT_BASE            (IDT_BASE+IDT_SIZE)     // GDT immediately follows IDT
#define GDT_LIMIT           (GDT_COUNT*DESC_SIZE-1)
#define GDT_SIZE            (GDT_LIMIT+1)

#define LDT_COUNT           2
#define LDT_BASE            (GDT_BASE+GDT_SIZE)     // LDT immediately follows GDT
#define LDT_LIMIT           (LDT_COUNT*DESC_SIZE-1)
#define LDT_SIZE            (LDT_LIMIT+1)

#define TSS_BASE            (LDT_BASE+LDT_SIZE)     // TSS immediately follows LDT
#define TSS_LIMIT           (TSS_BASE+TSS_SIZE-1)   // TSS_SIZE fixed, defined in x86.h

/* GDT Segment Selectors */
#define SEGSEL_NULL         (0x0)
#define SEGSEL_LDT          (0x08|DPL_KERNEL)
#define SEGSEL_KERNEL_CODE  (0x10|DPL_KERNEL)
#define SEGSEL_KERNEL_DATA  (0x18|DPL_KERNEL)
#define SEGSEL_USER_CODE    (0x20|DPL_USER)
#define SEGSEL_USER_DATA    (0x28|DPL_USER)
#define SEGSEL_TSS          (0x30|DPL_KERNEL)

extern BootInfo * const g_pBootInfo;

static void init_gdt(void);
static void init_idt(void);
static void init_ldt(void);
static void init_tss(void);

void init_cpu(void)
{
    init_gdt();
    init_idt();
    init_ldt();
    init_tss();
}

static void init_gdt(void)
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
    x86Desc *pKernelCs = get_desc(GDT_BASE, SEGSEL_KERNEL_CODE);
    x86Desc *pKernelDs = get_desc(GDT_BASE, SEGSEL_KERNEL_DATA);
    x86Desc *pUserCs = get_desc(GDT_BASE, SEGSEL_USER_CODE);
    x86Desc *pUserDs = get_desc(GDT_BASE, SEGSEL_USER_DATA);

    make_seg_desc(pKernelCs, DPL_KERNEL, 0x0, LIMIT_MAX, DESCTYPE_CODE_XR); // KERNEL_CS -> DPL0, Execute/Read
    make_seg_desc(pKernelDs, DPL_KERNEL, 0x0, LIMIT_MAX, DESCTYPE_DATA_RW); // KERNEL_DS -> DPL0, Read/Write
    make_seg_desc(pUserCs,   DPL_USER,   0x0, LIMIT_MAX, DESCTYPE_CODE_XR); // USER_CS   -> DPL3, Execute/Read
    make_seg_desc(pUserDs,   DPL_USER,   0x0, LIMIT_MAX, DESCTYPE_DATA_RW); // USER_DS   -> DPL3, Read/Write

    // Create GDT descriptor and use it to load GDTR, effectively "setting" the GDT
    PseudoDesc gdtDesc = { .base = GDT_BASE, .limit = GDT_LIMIT };
    lgdt(gdtDesc);

    // Reload segment registers with new segment selectors
    load_cs(SEGSEL_KERNEL_CODE);
    load_ds(SEGSEL_KERNEL_DATA);
    load_es(SEGSEL_KERNEL_DATA);
    load_fs(SEGSEL_NULL);
    load_gs(SEGSEL_NULL);
    load_ss(SEGSEL_KERNEL_DATA);
}

static void init_idt(void)
{
    idt_thunk excepts[NUM_EXCEPTION] =
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

    idt_thunk irqs[NUM_IRQ] =
    {
        _thunk_irq00h, _thunk_irq01h, _thunk_irq02h, _thunk_irq03h,
        _thunk_irq04h, _thunk_irq05h, _thunk_irq06h, _thunk_irq07h,
        _thunk_irq08h, _thunk_irq09h, _thunk_irq0Ah, _thunk_irq0Bh,
        _thunk_irq0Ch, _thunk_irq0Dh, _thunk_irq0Eh, _thunk_irq0Fh
    };

    // Zero IDT
    memset((void *) IDT_BASE, 0, IDT_SIZE);

    // Fill IDT
    int count = IDT_SIZE / sizeof(x86Desc);
    for (int idx = 0, e = 0, i = 0; idx < count; idx++) {
        x86Desc *desc = ((x86Desc *) IDT_BASE) + idx;
        e = idx - INT_EXCEPTION;
        i = idx - INT_IRQ;

        if (idx >= INT_EXCEPTION && e < NUM_EXCEPTION) {
            // interrupt gate for exceptions
            make_intr_gate(desc, SEGSEL_KERNEL_CODE, DPL_KERNEL, excepts[e]);
        }
        else if (idx >= INT_IRQ && i < NUM_IRQ) {
            // interrupt gate for device IRQs, we don't want other devices
            // interrupting this one's handler
            make_intr_gate(desc, SEGSEL_KERNEL_CODE, DPL_KERNEL, irqs[i]);
        }
        else if (idx == INT_SYSCALL) {
            // user-mode accessible trap gate, so devices can interrupt
            // system call
            make_trap_gate(desc, SEGSEL_KERNEL_CODE, DPL_USER, _thunk_syscall);
        }
        // else, keep it NULL! Will generate another exception
    }

    PseudoDesc idtDesc = { .base = IDT_BASE, .limit = IDT_LIMIT };
    lidt(idtDesc);
}

static void init_ldt(void)
{
    //
    // LDT loaded but not used
    //

    // Zero LDT
    memset((void *) LDT_BASE, 0, IDT_SIZE);

    // Create LDT entry in GDT using predefined LDT segment selector.
    void *pLdtDesc = get_desc(GDT_BASE, SEGSEL_LDT);
    make_ldt_desc(pLdtDesc, DPL_KERNEL, LDT_BASE, LDT_LIMIT);

    // Load LDTR
    lldt(SEGSEL_LDT);
}

static void init_tss(void)
{
    //
    // TSS used minimally; only when an interrupt occurs that changes CPU to
    // privilege level 0 (kernel mode) in order to locate the kernel stack.
    //

    // Zero TSS
    Tss * const TheTss = (Tss *) TSS_BASE;
    memset(TheTss, 0, TSS_SIZE);

    // Fill TSS with LDT segment selector, kernel stack pointer, and kernel stack segment selector
    TheTss->ldtSegSel = SEGSEL_LDT;
    TheTss->esp0 = (uint32_t) g_pBootInfo->stackBase;
    TheTss->ss0 = SEGSEL_KERNEL_DATA;

    // Add TSS entry to GDT
    void *pTssDesc = get_desc(GDT_BASE, SEGSEL_TSS);
    make_tss_desc(pTssDesc, DPL_KERNEL, TSS_BASE, TSS_LIMIT);

    // Load Task Register
    ltr(SEGSEL_TSS);
}
