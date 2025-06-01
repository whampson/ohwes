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
 *         File: include/cpu.h
 *      Created: January 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CPU_H
#define __CPU_H

#include <stdbool.h>
#include <i386/x86.h>
#include <kernel/kernel.h>

struct cpuid    // TODO: move to cpuid.h or something
{
    char vendor_id[13];         // e.g. "GenuineIntel"
    char family;                // cpu family
    char model;                 // cpu model
    char stepping;              // cpu stepping
    char type;                  // cpu type
    char level;                 // max parameter number
    uint32_t level_extended;    // max extended parameter number

    char brand_index;
    char brand_name[49];

    bool fpu_support;           // cpu contains an on-chip x87 FPU
    bool pse_support;           // large page support (CR4.PSE bit)
    bool pge_support;           // global page support (CR4.PGE bit)
    bool pat_support;           // page attribute table support (CR4.PAT bit)
    bool tsc_support;           // cpu has RDTSC instruction
    bool msr_support;           // cpu has RDMSR/WRMSR instrctions
};

bool cpu_has_cr4(void);
bool cpu_has_cpuid(void);
bool get_cpu_info(struct cpuid *cpuid_info);    // cpuid

static inline struct x86_pde * get_pgdir(void)
{
    uint32_t cr3; store_cr3(cr3);
    return (struct x86_pde *) KERNEL_ADDR(cr3);
}

static inline struct x86_desc * get_gdt(void)
{
    struct table_desc gdt_desc;
    __sgdt(gdt_desc);

    return (struct x86_desc *) KERNEL_ADDR(gdt_desc.base);
}

static inline struct x86_desc * get_idt(void)
{
    struct table_desc idt_desc;
    __sidt(idt_desc);

    return (struct x86_desc *) KERNEL_ADDR(idt_desc.base);
}

static inline struct tss * get_tss(uint16_t segsel)
{
    struct x86_desc *gdt = get_gdt();
    struct x86_desc *tss_desc = x86_get_desc(gdt, segsel);

    return (struct tss *) ((tss_desc->tss.basehi << 24) | tss_desc->tss.baselo);
}

static inline struct tss * get_curr_tss(void)
{
    uint16_t segsel; __str(segsel);
    return get_tss(segsel);
}

static inline int get_cpl()
{
    return getpl();
}

#endif // __CPU_H
