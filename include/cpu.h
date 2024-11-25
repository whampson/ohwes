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
 *         File: include/cpu.h
 *      Created: January 15, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __CPU_H
#define __CPU_H

#include <stdbool.h>
#include <x86.h>

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

#endif // __CPU_H
