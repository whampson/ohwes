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
 *         File: src/i386/kernel/cpu.c
 *      Created: December 24, 2023
 *       Author: Wes Hampson
 *
 * Intel x86 CPU initialization for 32-bit Protected Mode.
 * =============================================================================
 */

#include <i386/boot.h>
#include <i386/cpu.h>
#include <i386/interrupt.h>
#include <i386/paging.h>
#include <kernel/config.h>
#include <kernel/irq.h>
#include <kernel/mm.h>
#include <kernel/ohwes.h>

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

static void setup_ldt(void);
static void setup_tss(void);
static void setup_idt(void);
static void validate_gdt(void);

static struct x86_desc _ldt[1];
static struct tss _tss_table[2];

extern idt_thunk _exception_thunks[NR_EXCEPTIONS];
extern idt_thunk _irq_thunks[NR_IRQS];
extern idt_thunk _syscall_thunk;

void setup_cpu(void)
{
    validate_gdt();

    struct x86_desc *idt = get_idt();
    struct tss *tss_emerg = &_tss_table[0];
    struct tss *tss_kernl = &_tss_table[1];

    for (int i = 0; i < NR_EXCEPTIONS; i++) {
        struct x86_desc *idt_desc = &idt[EXCEPTION_BASE_VECTOR + i];
        idt_thunk entry = _exception_thunks[i];

        if (i == DOUBLE_FAULT) {
            // double fault, ensure we have a fresh stack
            make_task_gate(idt_desc, EMERG_TSS, KERNEL_PL);
            tss_emerg->cr3 = (uint32_t) __page_dir;
            tss_emerg->esp = KERNEL_ADDR(EMERG_STACK);
            tss_emerg->ebp = KERNEL_ADDR(EMERG_STACK);
            tss_emerg->eip = (uint32_t) entry;
            tss_emerg->cs = KERNEL_CS; tss_emerg->ss = KERNEL_DS;
            tss_emerg->ds = KERNEL_DS; tss_emerg->es = KERNEL_DS;
        }
        else {
            // exceptions, ensure int3 can be executed from user mode
            int pl = (i == BREAKPOINT) ? USER_PL : KERNEL_PL;
            make_trap_gate(idt_desc, KERNEL_CS, pl, entry);
        }
    }

    for (int i = 0; i < NR_IRQS; i++) {
        // device interrupts, disallow nested interrupts
        make_intr_gate(
            &idt[IRQ_BASE_VECTOR + i], KERNEL_CS,
            KERNEL_PL, _irq_thunks[i]);
    }

    // system call trap
    make_trap_gate(&idt[SYSCALL_VECTOR], KERNEL_CS, USER_PL, &_syscall_thunk);

    // TSS for kernel calls from user mode
    make_tss_desc(x86_get_desc(get_gdt(), KERNEL_TSS), KERNEL_PL, tss_kernl);
    tss_kernl->esp0 = KERNEL_ADDR(KERNEL_STACK);
    tss_kernl->ss0 = KERNEL_DS;
    __ltr(KERNEL_TSS);

    // dummy LDT descriptor so CPU doesn't freak out
    make_ldt_desc(
        x86_get_desc(get_gdt(), KERNEL_LDT),
        KERNEL_PL, (int) _ldt, sizeof(_ldt) - 1);
    __lldt(KERNEL_LDT);
}

static void validate_gdt(void)
{
    // verify that hardcoded GDT from setup is valid

    // TODO: panic if fucked up
    // TODO: verify base/limit in kernel space

    struct x86_desc *gdt = get_gdt();

    struct x86_desc *kernel_cs = x86_get_desc(gdt, KERNEL_CS);
    assert(kernel_cs->seg.type == DESCTYPE_CODE_XR || kernel_cs->seg.type == DESCTYPE_CODE_XRA);
    assert(kernel_cs->seg.dpl == KERNEL_PL);
    assert(kernel_cs->seg.db == 1);
    assert(kernel_cs->seg.s == 1);
    assert(kernel_cs->seg.g == 1);
    assert(kernel_cs->seg.p == 1);

    struct x86_desc *kernel_ds = x86_get_desc(gdt, KERNEL_DS);
    assert(kernel_ds->seg.type == DESCTYPE_DATA_RW || kernel_ds->seg.type == DESCTYPE_DATA_RWA);
    assert(kernel_ds->seg.dpl == KERNEL_PL);
    assert(kernel_ds->seg.db == 1);
    assert(kernel_ds->seg.s == 1);
    assert(kernel_ds->seg.g == 1);
    assert(kernel_ds->seg.p == 1);

    struct x86_desc *user_cs = x86_get_desc(gdt, USER_CS);
    assert(user_cs->seg.type == DESCTYPE_CODE_XR);
    assert(user_cs->seg.dpl == USER_PL);
    assert(user_cs->seg.db == 1);
    assert(user_cs->seg.s == 1);
    assert(user_cs->seg.g == 1);
    assert(user_cs->seg.p == 1);

    struct x86_desc *user_ds = x86_get_desc(gdt, USER_DS);
    assert(user_ds->seg.type == DESCTYPE_DATA_RW);
    assert(user_ds->seg.dpl == USER_PL);
    assert(user_ds->seg.db == 1);
    assert(user_ds->seg.s == 1);
    assert(user_ds->seg.g == 1);
    assert(user_ds->seg.p == 1);
}

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
    get_cpu_info(&cpu);

    return cpu.pse_support;
}

struct x86_desc * get_gdt(void)
{
    struct table_desc gdt_desc;
    __sgdt(gdt_desc);

    return (struct x86_desc *) KERNEL_ADDR(gdt_desc.base);
}

struct x86_desc * get_idt(void)
{
    struct table_desc idt_desc;
    __sidt(idt_desc);

    return (struct x86_desc *) KERNEL_ADDR(idt_desc.base);
}

struct tss * get_curr_tss(void)
{
    uint16_t segsel; __str(segsel);
    return get_tss(segsel);
}

struct tss * get_tss(uint16_t segsel)
{
    struct x86_desc *gdt = get_gdt();
    struct x86_desc *tss_desc = x86_get_desc(gdt, segsel);

    return (struct tss *) ((tss_desc->tss.basehi << 24) | tss_desc->tss.baselo);
}

struct x86_pde * get_pgdir(void)
{
    uint32_t cr3; store_cr3(cr3);
    return (struct x86_pde *) KERNEL_ADDR(cr3);
}

int get_cpl(void)
{
    struct segsel cs; store_cs(cs);
    return cs.rpl;
}

int get_rpl(uint16_t segsel)
{
    return ((struct segsel *) &segsel)->rpl;
}

//
// TODO: I want to rewrite this
//
bool get_cpu_info(struct cpuid *info)
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
