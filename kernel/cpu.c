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
