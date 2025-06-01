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
static void verify_gdt(void);

__data_segment static struct x86_desc _ldt[1];
__data_segment static struct tss _tss_table[2];

extern __noreturn void handle_double_fault(void);  // crash.c

// TODO: this is a bit of a janky way of managing kernel stacks
// but it'll work for now
void push_kernel_stack(void)
{
    struct tss *tss = get_curr_tss();
    tss->esp0 -= FRAME_SIZE;
    if (tss->esp0 <= KERNEL_ADDR(INT_STACK_LIMIT)) {
        panic("too many nested interrupts!");
    }
}

void pop_kernel_stack(void)
{
    struct tss *tss = get_curr_tss();
    tss->esp0 += FRAME_SIZE;
    if (tss->esp0 > KERNEL_ADDR(INT_STACK_BASE)) {
        panic("kernel stack underflow");
    }
}

void setup_cpu(void)
{
    setup_idt();
    setup_ldt();
    setup_tss();
    verify_gdt();

    // struct cpuid cpuid;
    // bool has_cpuid = get_cpu_info(&cpuid);
    // if (has_cpuid) {
    //     kprint("cpu: vendor=%s level=%02xh", cpuid.vendor_id, cpuid.level);
    //     if (cpuid.level >= 1) {
    //         kprint(" family=%02xh model=%02xh step=%02xh",
    //             cpuid.family, cpuid.model, cpuid.stepping);
    //         kprint("\ncpu: type=%02xh index=%02xh ext=%02xh",
    //             cpuid.type, cpuid.brand_index, cpuid.level_extended);
    //     }
    //     if (cpuid.level_extended >= 0x80000004) {
    //         kprint("\ncpu: name='%s'", cpuid.brand_name);
    //     }
    //     kprint("\n");
    // }
}

void setup_idt(void)
{
    extern idt_thunk exception_thunks[NR_EXCEPTIONS];
    extern idt_thunk irq_thunks[NR_IRQS];
    extern idt_thunk syscall_thunk;

    struct x86_desc *idt = get_idt();

    for (int i = 0; i < NR_EXCEPTIONS; i++) {
        // interrupt gate for system exceptions;
        // each handler must explicitly enable interrupts if it wants them
        make_intr_gate(&idt[VEC_INTEL + i], KERNEL_CS, KERNEL_PL, exception_thunks[i]);
    }

    for (int i = 0; i < NR_IRQS; i++) {
        // interrupt gate for device IRQs; prevent nested IRQs
        make_intr_gate(&idt[VEC_IRQ + i], KERNEL_CS, KERNEL_PL, irq_thunks[i]);
    }

    // user-mode interrupt gate for breakpoint handler
    // so user-mode programs can trigger it
    make_intr_gate(&idt[EXCEPTION_BP], KERNEL_CS, USER_PL, exception_thunks[EXCEPTION_BP]);

    // task gate for double fault handler to force a stack switch
    // so we don't lose original exception's stack
    make_task_gate(&idt[EXCEPTION_DF], _TSS1_SEGMENT, KERNEL_PL);

    // trap gate for system calls; device interrupts are OK
    make_trap_gate(&idt[VEC_SYSCALL], KERNEL_CS, USER_PL, &syscall_thunk);
}

void setup_ldt(void)
{
    // no LDT in use (only contains null entry);
    // make a dummy descriptor so CPU doesn't freak out
    make_ldt_desc(
        x86_get_desc(get_gdt(), _LDT_SEGMENT),
        KERNEL_PL, (uintptr_t) _ldt, sizeof(_ldt) - 1);

    // load LDT descriptor register
    __lldt(_LDT_SEGMENT);
}

static void setup_tss(void)
{
    // normal interrupt TSS
    make_tss_desc(
        x86_get_desc(get_gdt(), _TSS0_SEGMENT),
        KERNEL_PL, (uintptr_t) &_tss_table[0]);
    struct tss *tss = get_tss(_TSS0_SEGMENT);
    assert(tss == &_tss_table[0]);
    tss->esp0 = KERNEL_ADDR(INT_STACK_BASE);
    tss->ss0 = KERNEL_DS;

    // double fault TSS, used to force a stack switch so we don't lose
    // initial fault's stack
    make_tss_desc(
        x86_get_desc(get_gdt(), _TSS1_SEGMENT),
        KERNEL_PL, (uintptr_t) &_tss_table[1]);

    struct tss *crash_tss = get_tss(_TSS1_SEGMENT);
    assert(crash_tss == &_tss_table[1]);
    crash_tss->eip = (uint32_t) handle_double_fault;
    crash_tss->esp = KERNEL_ADDR(DOUBLE_FAULT_STACK);
    crash_tss->ebp = KERNEL_ADDR(DOUBLE_FAULT_STACK);
    crash_tss->cs = KERNEL_CS;
    crash_tss->ds = KERNEL_DS;
    crash_tss->es = KERNEL_DS;
    crash_tss->ss = KERNEL_DS;
    crash_tss->cr3 = KERNEL_PGDIR;

    // load task register
    __ltr(_TSS0_SEGMENT);
}

static void verify_gdt(void)
{
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

    struct x86_desc *ldt_desc = x86_get_desc(gdt, _LDT_SEGMENT);
    assert(ldt_desc->seg.type == DESCTYPE_LDT);
    assert(ldt_desc->seg.dpl == KERNEL_PL);
    assert(ldt_desc->seg.s == 0);
    assert(ldt_desc->seg.g == 0);
    assert(ldt_desc->seg.p == 1);
    // TODO: verify base/limit in kernel space

    struct x86_desc *tss_desc = x86_get_desc(gdt, _TSS0_SEGMENT);
    assert(tss_desc->tss.type == DESCTYPE_TSS32_BUSY);
    assert(tss_desc->tss.dpl == KERNEL_PL);
    assert(tss_desc->tss.g == 0);
    assert(tss_desc->tss.p == 1);
    // TODO: verify base/limit in kernel space
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
