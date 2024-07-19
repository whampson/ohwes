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

// #if DEBUG
// #define CHATTY               1
// #endif

// //
// // x86 Descriptor table and TSS geometry.
// //
// // IDT
// #define IDT_COUNT           256
// #define IDT_BASE            SYSTEM_CPU_PAGE
// #define IDT_LIMIT           (IDT_COUNT*DESC_SIZE-1)
// #define IDT_SIZE            (IDT_LIMIT+1)
// // GDT                                              //
// #define GDT_COUNT           8                       //
// #define GDT_BASE            (IDT_BASE+IDT_SIZE)     // GDT immediately follows IDT
// #define GDT_LIMIT           (GDT_COUNT*DESC_SIZE-1)
// #define GDT_SIZE            (GDT_LIMIT+1)
// // LDT
// #define LDT_COUNT           2
// #define LDT_BASE            (GDT_BASE+GDT_SIZE)     // LDT immediately follows GDT
// #define LDT_LIMIT           (LDT_COUNT*DESC_SIZE-1)
// #define LDT_SIZE            (LDT_LIMIT+1)
// // TSS
// #define TSS_BASE            (LDT_BASE+LDT_SIZE)     // TSS immediately follows LDT
// #define TSS_LIMIT           (TSS_SIZE-1)            // TSS_SIZE is fixed, defined in x86.h

// static_assert(IDT_BASE + IDT_LIMIT < GDT_BASE, "IDT overlaps GDT!");
// static_assert(GDT_BASE + GDT_LIMIT < LDT_BASE, "GDT overlaps LDT!");
// static_assert(LDT_BASE + LDT_LIMIT < TSS_BASE, "LDT overlaps TSS!");
// static_assert(IDT_SIZE + GDT_SIZE + LDT_SIZE + TSS_SIZE <= PAGE_SIZE, "CPU data runs into next page!");

//
// CPUID.EAX=01h EAX return fields.
//
#define CPUID_STEPPING_SHIFT    0
#define CPUID_STEPPING_MASK     0x0F
#define CPUID_MODEL_SHIFT       4
#define CPUID_MODEL_MASK        0x0F
#define CPUID_FAMILY_SHIFT      8
#define CPUID_FAMILY_MASK       0x0F
#define CPUID_TYPE_SHIFT        12
#define CPUID_TYPE_MASK         0x03
#define CPUID_EXT_MODEL_SHIFT   16
#define CPUID_EXT_MODEL_MASK    0x0F
#define CPUID_EXT_FAMILY_SHIFT  20
#define CPUID_EXT_FAMILY_MASK   0xFF

//
// CPUID.EAX=01h EDX return bits.
//
#define CPUID_FPU               (1 << 0)
#define CPUID_PSE               (1 << 3)
#define CPUID_TSC               (1 << 4)
#define CPUID_MSR               (1 << 5)
#define CPUID_PAE               (1 << 6)
#define CPUID_PGE               (1 << 13)
#define CPUID_PAT               (1 << 16)

/**
 * Gets a Segment Descriptor from a descriptor table.
 *
 * @param table a pointer to the descriptor table
 * @param segsel the segment selector used to index the table
 * @return a pointer to the segment descriptor specified by the segment selector
 */
#define get_desc(table,segsel) \
    (&((struct x86_desc*)(table))[(segsel)>>3])

// struct x86_desc * cpu_get_desc(uint16_t segsel)
// {
//     struct segsel *ss = (struct segsel *) &segsel;

//     if (ss->ti) {
//         if (segsel > LDT_LIMIT) {
//             panic("LDT segment selector out of range - %04X", segsel);
//         }
//         return get_desc(LDT_BASE, segsel);
//     }

//     if (segsel > GDT_LIMIT) {
//         panic("GDT segment selector out of range - %04X", segsel);
//     }
//     return get_desc(GDT_BASE, segsel);
// }

// struct tss * cpu_get_tss(void)
// {
//     return (struct tss *) TSS_BASE;
// }

bool cpu_has_cpuid(void)
{
    uint32_t flags;

    cli_save(flags);
    flags |= EFLAGS_ID;         // attempt to set ID flags
    restore_flags(flags);
    cli_save(flags);            // readback

    return flags & EFLAGS_ID;   // if it's still set, CPUID supported
}

bool cpu_has_cr4(void)
{
    // Large pages are enabled by the PSE bit in CR4. The presence of this bit
    // is determined by a call to CPUID EAX=01h. Thus, if  the CPU has large
    // page support, the CR4 register must also be present.

    struct cpuid cpu;
    get_cpuid(&cpu);

    return cpu.pse_support;
}

bool get_cpuid(struct cpuid *info)
{
    uint8_t ext_family, ext_model;
    uint32_t eax, ebx, ecx, edx;
    uint32_t char_buf[4];

    zeromem(info, sizeof(struct cpuid));
    zeromem(char_buf, sizeof(char_buf));

    if (!cpu_has_cpuid()) {
        return false;        // return after zeroing struct so support bools are false
    }

    static_assert(sizeof(char_buf) <= sizeof(info->brand_name), "brand name buffer too small");

    __cpuid(0x0, eax, char_buf[0], char_buf[2], char_buf[1]);
    memcpy(info->vendor_id, char_buf, 12);
    info->vendor_id[12] = '\0';
    info->level = eax;


    if (info->level >= 1) {
        __cpuid(0x1, eax, ebx, ecx, edx);

        info->type = (eax >> CPUID_TYPE_SHIFT) & CPUID_TYPE_MASK;
        info->family = (eax >> CPUID_FAMILY_SHIFT) & CPUID_FAMILY_MASK;
        info->model = (eax >> CPUID_MODEL_SHIFT) & CPUID_MODEL_MASK;
        info->stepping = (eax >> CPUID_STEPPING_SHIFT) & CPUID_STEPPING_MASK;
        ext_model = (eax >> CPUID_EXT_MODEL_SHIFT) & CPUID_EXT_MODEL_MASK;
        ext_family = (eax >> CPUID_EXT_FAMILY_SHIFT) & CPUID_EXT_FAMILY_MASK;

        if (info->family == 0x0F) {
            info->family += ext_family;
        }
        if (info->model == 0x06 || info->model) {
            info->model += (ext_model << 4);
        }

        info->fpu_support = edx & CPUID_FPU;
        info->pse_support = edx & CPUID_PSE;
        info->pge_support = edx & CPUID_PGE;
        info->pat_support = edx & CPUID_PAT;
        info->tsc_support = edx & CPUID_TSC;
        info->msr_support = edx & CPUID_MSR;
        info->brand_index = ebx & 0xFF;
    }

    __cpuid(0x80000000, eax, ebx, ecx, edx);
    if (eax & 0x80000000) {
        info->level_extended = eax;
    }

    if (info->level_extended >= 0x80000004) {
        __cpuid(0x80000002, char_buf[0], char_buf[1], char_buf[2], char_buf[3]);
        memcpy(info->brand_name, char_buf, sizeof(char_buf));
        __cpuid(0x80000003, char_buf[0], char_buf[1], char_buf[2], char_buf[3]);
        memcpy(info->brand_name + 16, char_buf, sizeof(char_buf));
        __cpuid(0x80000004, char_buf[0], char_buf[1], char_buf[2], char_buf[3]);
        memcpy(info->brand_name + 32, char_buf, sizeof(char_buf));
        info->brand_name[48] = '\0';
    }

    return true;
}

static void init_gdt(const struct boot_info * const info);
static void init_idt(const struct boot_info * const info);
static void init_ldt(const struct boot_info * const info);
static void init_tss(const struct boot_info * const info);

extern __fastcall void _thunk_except00h(void);
extern __fastcall void _thunk_except01h(void);
extern __fastcall void _thunk_except02h(void);
extern __fastcall void _thunk_except03h(void);
extern __fastcall void _thunk_except04h(void);
extern __fastcall void _thunk_except05h(void);
extern __fastcall void _thunk_except06h(void);
extern __fastcall void _thunk_except07h(void);
extern __fastcall void _thunk_except08h(void);
extern __fastcall void _thunk_except09h(void);
extern __fastcall void _thunk_except0Ah(void);
extern __fastcall void _thunk_except0Bh(void);
extern __fastcall void _thunk_except0Ch(void);
extern __fastcall void _thunk_except0Dh(void);
extern __fastcall void _thunk_except0Eh(void);
extern __fastcall void _thunk_except0Fh(void);
extern __fastcall void _thunk_except10h(void);
extern __fastcall void _thunk_except11h(void);
extern __fastcall void _thunk_except12h(void);
extern __fastcall void _thunk_except13h(void);
extern __fastcall void _thunk_except14h(void);
extern __fastcall void _thunk_except15h(void);
extern __fastcall void _thunk_except16h(void);
extern __fastcall void _thunk_except17h(void);
extern __fastcall void _thunk_except18h(void);
extern __fastcall void _thunk_except19h(void);
extern __fastcall void _thunk_except1Ah(void);
extern __fastcall void _thunk_except1Bh(void);
extern __fastcall void _thunk_except1Ch(void);
extern __fastcall void _thunk_except1Dh(void);
extern __fastcall void _thunk_except1Eh(void);
extern __fastcall void _thunk_except1Fh(void);
extern __fastcall void _thunk_irq00h(void);
extern __fastcall void _thunk_irq01h(void);
extern __fastcall void _thunk_irq02h(void);
extern __fastcall void _thunk_irq03h(void);
extern __fastcall void _thunk_irq04h(void);
extern __fastcall void _thunk_irq05h(void);
extern __fastcall void _thunk_irq06h(void);
extern __fastcall void _thunk_irq07h(void);
extern __fastcall void _thunk_irq08h(void);
extern __fastcall void _thunk_irq09h(void);
extern __fastcall void _thunk_irq0Ah(void);
extern __fastcall void _thunk_irq0Bh(void);
extern __fastcall void _thunk_irq0Ch(void);
extern __fastcall void _thunk_irq0Dh(void);
extern __fastcall void _thunk_irq0Eh(void);
extern __fastcall void _thunk_irq0Fh(void);
extern __fastcall void _thunk_syscall(void);
extern __fastcall void _thunk_test(void);

void init_cpu(const struct boot_info * const info)
{
    // init_gdt(info);
    // init_idt(info);
    // init_ldt(info);
    // init_tss(info);

// #ifdef CHATTY
//     struct cpuid cpuid;
//     if (get_cpuid(&cpuid)) {
//         kprint("cpuid: family=%d,model=%d,stepping=%d,type=%d,level=%d,ext_level=%02lXh\n"
//                "cpuid: index=%02Xh,ven=%s,name=%s\n"
//                "cpuid: fpu=%d,pse=%d,pge=%d,pat=%d,tsc=%d,msr=%d\n",
//             cpuid.family, cpuid.model, cpuid.stepping, cpuid.type,cpuid.level,
//             cpuid.level_extended, cpuid.brand_index, cpuid.vendor_id, cpuid.brand_name,
//             cpuid.fpu_support, cpuid.pse_support, cpuid.pge_support,
//             cpuid.pat_support, cpuid.tsc_support, cpuid.msr_support);
//     }
// #endif
}

static void init_gdt(const struct boot_info * const info)
{
//     //
//     // Global Descriptor Table (GDT) Initialization
//     //
//     // See Intel Software Developer's Manual, Vol 3A
//     //     sections 3.4 & 3.5
//     //

//     // Zero GDT
//     memset((void *) GDT_BASE, 0, GDT_SIZE);

//     // Open up all of memory (0-4G) for use by kernel and user
//     struct x86_desc *kernel_cs = get_desc(GDT_BASE, KERNEL_CS);
//     struct x86_desc *kernel_ds = get_desc(GDT_BASE, KERNEL_DS);
//     struct x86_desc *user_cs = get_desc(GDT_BASE, USER_CS);
//     struct x86_desc *user_ds = get_desc(GDT_BASE, USER_DS);

//     make_seg_desc(kernel_cs, KERNEL_PL, 0x0, LIMIT_MAX, DESCTYPE_CODE_XR);  // KERNEL_CS -> DPL0, Execute/Read
//     make_seg_desc(kernel_ds, KERNEL_PL, 0x0, LIMIT_MAX, DESCTYPE_DATA_RW);  // KERNEL_DS -> DPL0, Read/Write
//     make_seg_desc(user_cs,   USER_PL,   0x0, LIMIT_MAX, DESCTYPE_CODE_XR);  // USER_CS   -> DPL3, Execute/Read
//     make_seg_desc(user_ds,   USER_PL,   0x0, LIMIT_MAX, DESCTYPE_DATA_RW);  // USER_DS   -> DPL3, Read/Write

//     // Create GDT descriptor and use it to load GDTR, effectively "setting" the GDT
//     struct pseudo_desc gdt_desc = { .base = GDT_BASE, .limit = GDT_LIMIT };
//     __lgdt(gdt_desc);

//     // Reload segment registers with new segment selectors
//     write_cs(KERNEL_CS);
//     write_ds(KERNEL_DS);
//     write_es(KERNEL_DS);
//     write_ss(KERNEL_DS);
//     write_fs(0);
//     write_gs(0);
}

static void init_idt(const struct boot_info * const info)
{
//     int count;
//     struct x86_desc *desc;

//     static const idt_thunk excepts[NUM_EXCEPTIONS] =
//     {
//         _thunk_except00h, _thunk_except01h, _thunk_except02h, _thunk_except03h,
//         _thunk_except04h, _thunk_except05h, _thunk_except06h, _thunk_except07h,
//         _thunk_except08h, _thunk_except09h, _thunk_except0Ah, _thunk_except0Bh,
//         _thunk_except0Ch, _thunk_except0Dh, _thunk_except0Eh, _thunk_except0Fh,
//         _thunk_except10h, _thunk_except11h, _thunk_except12h, _thunk_except13h,
//         _thunk_except14h, _thunk_except15h, _thunk_except16h, _thunk_except17h,
//         _thunk_except18h, _thunk_except19h, _thunk_except1Ah, _thunk_except19h,
//         _thunk_except1Ch, _thunk_except1Ch, _thunk_except1Eh, _thunk_except1Fh
//     };

//     static const idt_thunk irqs[NUM_IRQS] =
//     {
//         _thunk_irq00h, _thunk_irq01h, _thunk_irq02h, _thunk_irq03h,
//         _thunk_irq04h, _thunk_irq05h, _thunk_irq06h, _thunk_irq07h,
//         _thunk_irq08h, _thunk_irq09h, _thunk_irq0Ah, _thunk_irq0Bh,
//         _thunk_irq0Ch, _thunk_irq0Dh, _thunk_irq0Eh, _thunk_irq0Fh
//     };

//     // Zero IDT
//     memset((void *) IDT_BASE, 0, IDT_SIZE);

//     // Fill IDT
//     count = IDT_SIZE / sizeof(struct x86_desc);
//     assert(count == 256);

//     for (int vec = 0, n = 0; vec < count; vec++) {
//         desc = ((struct x86_desc *) IDT_BASE) + vec;
//         if (vec >= VEC_INTEL && vec < VEC_INTEL + NUM_EXCEPTIONS) {
//             n = vec - VEC_INTEL;
//             make_trap_gate(desc, KERNEL_CS, KERNEL_PL, excepts[n]);
//         }
//         else if (vec >= VEC_DEVICEIRQ && vec < VEC_DEVICEIRQ + NUM_IRQS) {
//             n = vec - VEC_DEVICEIRQ;
//             make_intr_gate(desc, KERNEL_CS, KERNEL_PL, irqs[n]);
//         }
//         else if (vec == VEC_SYSCALL) {
//             make_trap_gate(desc, KERNEL_CS, USER_PL, _thunk_syscall);
//         }
//         // else, keep vector NULL; this will generate a double-fault exception
//     }

//     // Load the IDTR
//     struct pseudo_desc idt_desc = { .base = IDT_BASE, .limit = IDT_LIMIT };
//     __lidt(idt_desc);
}

static void init_ldt(const struct boot_info * const info)
{
    // //
    // // LDT loaded but not used
    // //

    // // Zero LDT
    // memset((void *) LDT_BASE, 0, LDT_SIZE);

    // // Create LDT entry in GDT using predefined LDT segment selector.
    // void *ldt_desc = get_desc(GDT_BASE, _GDT_LOCALDESC);
    // make_ldt_desc(ldt_desc, KERNEL_PL, LDT_BASE, LDT_LIMIT);

    // // Load LDTR
    // __lldt(_GDT_LOCALDESC);
}

static void init_tss(const struct boot_info * const info)
{
    // //
    // // TSS used minimally; only when an interrupt occurs that changes CPU to
    // // privilege level 0 (kernel mode) in order to locate the kernel stack.
    // // We don't keep one for every process like Intel wants us to.
    // //

    // // Zero TSS
    // struct tss * const tss = (struct tss *) TSS_BASE;
    // memset(tss, 0, TSS_SIZE);

    // // Fill TSS with LDT segment selector, kernel stack pointer,
    // // and kernel stack segment selector
    // tss->ldt_segsel = _GDT_LOCALDESC;
    // tss->esp0 = KERNEL_BASE;
    // tss->ss0 = KERNEL_DS;

    // // Add TSS entry to GDT
    // void *pTssDesc = get_desc(GDT_BASE, _GDT_TASKSTATE);
    // make_tss_desc(pTssDesc, KERNEL_PL, TSS_BASE, TSS_LIMIT);

    // // Load Task Register
    // __ltr(_GDT_TASKSTATE);
}
