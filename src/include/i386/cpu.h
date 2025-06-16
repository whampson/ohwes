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
#include <i386/interrupt.h>
#include <i386/x86.h>

/**
 * CPU Privilege Level
 */
enum pl {
    KERNEL_PL = 0,
    USER_PL = 3,
};

#define getpl()     get_cpl()       // TODO: remove

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

struct cpu_state {
    struct iregs iregs;
    uint32_t cr0;
    uint32_t cr2;
    uint32_t cr3;
    uint32_t cr4;
    uint64_t gdtr;
    uint64_t idtr;
    uint16_t ldtr;
    uint16_t tr;
};

bool cpu_has_cr4(void);
bool cpu_has_cpuid(void);
bool get_cpu_info(struct cpuid *cpuid_info);    // cpuid

struct x86_desc * get_gdt(void);
struct x86_desc * get_idt(void);
struct tss * get_tss(void);
struct tss * get_tss_from_gdt(uint16_t segsel);
struct x86_pde * get_pgdir(void);

// call these after receiving an interrupt
int get_cpl(void);
int get_rpl(uint16_t segsel);
bool pl_changed(const struct iregs *regs);
uint32_t get_esp(const struct iregs *regs);
uint16_t get_ss(const struct iregs *regs);

#endif // __CPU_H
