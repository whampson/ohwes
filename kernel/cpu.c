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

#include <ohwes.h>
#include <boot.h>
#include <cpu.h>
#include <interrupt.h>
#include <irq.h>
#include <paging.h>

#define CPU_DATA_AREA       0x1000

//
// x86 Descriptor table and TSS geometry.
//
// IDT
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
static_assert(IDT_SIZE + GDT_SIZE + LDT_SIZE + TSS_SIZE <= PAGE_SIZE, "CPU data runs into next page!");

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
            panic("LDT segment selector out of range - %04X", segsel);
        }
        return get_desc(LDT_BASE, segsel);
    }

    if (segsel > GDT_LIMIT) {
        panic("GDT segment selector out of range - %04X", segsel);
    }
    return get_desc(GDT_BASE, segsel);
}

struct cpu_info g_cpu_info;

const struct cpu_info * get_cpu_info(void)
{
    return &g_cpu_info;
}

bool has_cr4(void)
{
    // Large pages are enabled by the PSE bit in CR4. The presence of this bit
    // is determined by a call to CPUID EAX=01h, performed in init_cpu. Thus, if
    // the CPU has large page support, the CR4 register must also be present.
    return g_cpu_info.large_page_support;
}

static void init_gdt(const struct boot_info * const info);
static void init_idt(const struct boot_info * const info);
static void init_ldt(const struct boot_info * const info);
static void init_tss(const struct boot_info * const info);

#define cpuid(cmd,a,b,c,d)                              \
    __asm__ volatile (                                  \
        "cpuid"                                         \
            : "=a"(a), "=b"(b), "=c"(c), "=d"(d)        \
            : "a"(cmd)                                  \
    )                                                   \

#define CPUID_PSE   (1 << 3)
#define CPUID_RDTSC (1 << 4)
#define CPUID_RDMSR (1 << 5)
#define CPUID_PAE   (1 << 6)
#define CPUID_PGE   (1 << 13)
#define CPUID_PAT   (1 << 16)

void init_cpu(const struct boot_info * const info)
{
    uint32_t eax, ebx, ecx, edx;

    zeromem(&g_cpu_info, sizeof(struct cpu_info));

    init_gdt(info);
    init_idt(info);
    init_ldt(info);
    init_tss(info);

    if (has_cpuid()) {
        const int FeatureInfo = 0x01;
        cpuid(FeatureInfo, eax, ebx, ecx, edx);
        kprint("cpuid(%02Xh): eax=%08X ebx=%08X, ecx=%08X, edx=%08X\n",
             FeatureInfo, eax, ebx, ecx, edx);

        g_cpu_info.large_page_support = (edx & CPUID_PSE);
        g_cpu_info.rdtsc = (edx & CPUID_RDTSC);
        g_cpu_info.rdmsr = (edx & CPUID_RDMSR);
        g_cpu_info.pae = (edx & CPUID_PAE);
        g_cpu_info.pge = (edx & CPUID_PGE);
        g_cpu_info.pat = (edx & CPUID_PAT);
    }
}

static void init_gdt(const struct boot_info * const info)
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
    struct x86_desc *kernel_cs = get_desc(GDT_BASE, KERNEL_CS);
    struct x86_desc *kernel_ds = get_desc(GDT_BASE, KERNEL_DS);
    struct x86_desc *user_cs = get_desc(GDT_BASE, USER_CS);
    struct x86_desc *user_ds = get_desc(GDT_BASE, USER_DS);

    make_seg_desc(kernel_cs, KERNEL_PL, 0x0, LIMIT_MAX, DESCTYPE_CODE_XR);  // KERNEL_CS -> DPL0, Execute/Read
    make_seg_desc(kernel_ds, KERNEL_PL, 0x0, LIMIT_MAX, DESCTYPE_DATA_RW);  // KERNEL_DS -> DPL0, Read/Write
    make_seg_desc(user_cs,   USER_PL,   0x0, LIMIT_MAX, DESCTYPE_CODE_XR);  // USER_CS   -> DPL3, Execute/Read
    make_seg_desc(user_ds,   USER_PL,   0x0, LIMIT_MAX, DESCTYPE_DATA_RW);  // USER_DS   -> DPL3, Read/Write

    // Create GDT descriptor and use it to load GDTR, effectively "setting" the GDT
    struct pseudo_desc gdt_desc = { .base = GDT_BASE, .limit = GDT_LIMIT };
    lgdt(gdt_desc);

    // Reload segment registers with new segment selectors
    write_cs(KERNEL_CS);
    write_ds(KERNEL_DS);
    write_es(KERNEL_DS);
    write_ss(KERNEL_DS);
    write_fs(0);
    write_gs(0);
}

static void init_idt(const struct boot_info * const info)
{
    int count;
    struct x86_desc *desc;

    static const idt_thunk excepts[NUM_EXCEPTIONS] =
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

    static const idt_thunk irqs[NUM_IRQS] =
    {
        _thunk_irq00h, _thunk_irq01h, _thunk_irq02h, _thunk_irq03h,
        _thunk_irq04h, _thunk_irq05h, _thunk_irq06h, _thunk_irq07h,
        _thunk_irq08h, _thunk_irq09h, _thunk_irq0Ah, _thunk_irq0Bh,
        _thunk_irq0Ch, _thunk_irq0Dh, _thunk_irq0Eh, _thunk_irq0Fh
    };

    // Zero IDT
    memset((void *) IDT_BASE, 0, IDT_SIZE);

    // Fill IDT
    count = IDT_SIZE / sizeof(struct x86_desc);
    assert(count == 256);

    for (int vec = 0, n = 0; vec < count; vec++) {
        desc = ((struct x86_desc *) IDT_BASE) + vec;
        if (vec >= VEC_INTEL && vec < VEC_INTEL + NUM_EXCEPTIONS) {
            n = vec - VEC_INTEL;
            make_trap_gate(desc, KERNEL_CS, KERNEL_PL, excepts[n]);
        }
        else if (vec >= VEC_DEVICEIRQ && vec < VEC_DEVICEIRQ + NUM_IRQS) {
            n = vec - VEC_DEVICEIRQ;
            make_intr_gate(desc, KERNEL_CS, KERNEL_PL, irqs[n]);
        }
        else if (vec == VEC_SYSCALL) {
            make_trap_gate(desc, KERNEL_CS, USER_PL, _thunk_syscall);
        }
        // else, keep vector NULL; this will generate a double-fault exception
    }

    // Load the IDTR
    struct pseudo_desc idt_desc = { .base = IDT_BASE, .limit = IDT_LIMIT };
    lidt(idt_desc);
}

static void init_ldt(const struct boot_info * const info)
{
    //
    // LDT loaded but not used
    //

    // Zero LDT
    memset((void *) LDT_BASE, 0, IDT_SIZE);

    // Create LDT entry in GDT using predefined LDT segment selector.
    void *ldt_desc = get_desc(GDT_BASE, _GDT_LOCALDESC);
    make_ldt_desc(ldt_desc, KERNEL_PL, LDT_BASE, LDT_LIMIT);

    // Load LDTR
    lldt(_GDT_LOCALDESC);
}

static void init_tss(const struct boot_info * const info)
{
    //
    // TSS used minimally; only when an interrupt occurs that changes CPU to
    // privilege level 0 (kernel mode) in order to locate the kernel stack.
    // We don't keep one for every process like Intel wants us to.
    //

    // Zero TSS
    struct tss * const tss = (struct tss *) TSS_BASE;
    memset(tss, 0, TSS_SIZE);

    // Fill TSS with LDT segment selector, kernel stack pointer,
    // and kernel stack segment selector
    tss->ldt_segsel = _GDT_LOCALDESC;
    tss->esp0 = info->stack;   // TODO: don't forget to modify this when task switching!
    tss->ss0 = KERNEL_DS;

    // Add TSS entry to GDT
    void *pTssDesc = get_desc(GDT_BASE, _GDT_TASKSTATE);
    make_tss_desc(pTssDesc, KERNEL_PL, TSS_BASE, TSS_LIMIT);

    // Load Task Register
    ltr(_GDT_TASKSTATE);
}
