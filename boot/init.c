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
 *         File: boot/init.c
 *      Created: March 26, 2023
 *       Author: Wes Hampson
 *
 * x86 Protected Mode boot code.
 * =============================================================================
 */

#include "boot.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <os/console.h>

/* Segment Selectors */
#define SEGSEL_NULL         (0x0)
#define SEGSEL_LDT          (0x08|DPL_KERNEL)
#define SEGSEL_KERNEL_CODE  (0x10|DPL_KERNEL)
#define SEGSEL_KERNEL_DATA  (0x18|DPL_KERNEL)
#define SEGSEL_USER_CODE    (0x20|DPL_USER)
#define SEGSEL_USER_DATA    (0x28|DPL_USER)
#define SEGSEL_TSS          (0x30|DPL_KERNEL)

#define KERNEL_STACK        0x7C00

SegDesc * const g_gdt = (SegDesc *) GDT_BASE;
SegDesc * const g_idt = (SegDesc *) IDT_BASE;
SegDesc * const g_ldt = (SegDesc *) LDT_BASE;
struct Tss * const g_tss = (struct Tss *) TSS_BASE;

DescReg g_gdtDesc = { GDT_SIZE-1, GDT_BASE };
DescReg g_idtDesc = { IDT_SIZE-1, IDT_BASE };

void InitGdt();
void InitIdt();
void InitLdt();
void InitTss();
void irq_init();

void PrintHardwareInfo()
{
    const HwFlags *hwFlags = (HwFlags *) &g_HwFlags;
    printf("boot: diskette drive? %s\n", hwFlags->HasDisketteDrive ? "yes" : "no");
    printf("boot: coprocessor? %s\n", hwFlags->HasCoprocessor ? "yes" : "no");
    printf("boot: PS/2 mouse? %s\n", hwFlags->HasPs2Mouse ? "yes" : "no");
    printf("boot: game port? %s\n", hwFlags->HasGamePort ? "yes" : "no");
    printf("boot: num serial ports = %d\n", hwFlags->NumSerialPorts);
    printf("boot: num parallel ports = %d\n", hwFlags->NumParallelPorts);
    printf("boot: num secondary diskette drives = %d\n", hwFlags->NumOtherDisketteDrives);
    printf("boot: video mode = ");
    switch (hwFlags->VideoMode) {
        case HWFLAGS_VIDEOMODE_40x25:       printf("40x25\n"); break;
        case HWFLAGS_VIDEOMODE_80x25:       printf("80x25\n"); break;
        case HWFLAGS_VIDEOMODE_80x25_MONO:  printf("80x25 (monochrome)\n"); break;
        default:                            printf("(invalid)\n"); break;
    }
}

void PrintMemoryInfo()
{
    printf("boot: A20 ");
    switch (g_A20Method) {
        case A20METHOD_NONE:        printf("enabled\n"); break;
        case A20METHOD_KEYBOARD:    printf("enabled via PS/2 keyboard controller\n"); break;
        case A20METHOD_PORT92h:     printf("enabled via I/O port 92h\n"); break;
        case A20METHOD_BIOS:        printf("enabled via BIOS INT=15h,AX=2401h\n"); break;
        default:                    printf("(invalid)"); break;
    }
    printf("boot: g_RamLo_Legacy = %d\n", g_RamLo_Legacy);
    printf("boot: g_RamHi_Legacy = %d\n", g_RamHi_Legacy);
    printf("boot: g_RamLo_E801h = %d\n", g_RamLo_E801h);
    printf("boot: g_RamHi_E801h = %d\n", g_RamHi_E801h << 6);    // 64K pages to 1K pages

    if (g_HasAcpiMemoryMap) {
        const AcpiMemoryMapEntry *memMap = g_AcpiMemoryMap;
        do {
            if (memMap->Length > 0) {
                printf("boot: BIOS-E820h: %08x-%08x ",
                    (uint32_t) memMap->Base,
                    (uint32_t) memMap->Base + memMap->Length - 1);

                switch (memMap->Type) {
                    case ACPI_MMAP_TYPE_USABLE:     printf("usable\n"); break;
                    case ACPI_MMAP_TYPE_RESERVED:   printf("reserved\n"); break;
                    case ACPI_MMAP_TYPE_ACPI:       printf("ACPI\n"); break;
                    case ACPI_MMAP_TYPE_ACPI_NVS:   printf("ACPI NV\n"); break;
                    case ACPI_MMAP_TYPE_BAD:        printf("bad\n"); break;
                    default:                        printf("reserved (%d)\n", memMap->Type); break;
                }
            }
        } while ((memMap++)->Type != ACPI_MMAP_TYPE_INVALID);
    }
}

void init()
{
    cli();

    PrintHardwareInfo();
    PrintMemoryInfo();

    InitIdt();
    InitGdt();
    // InitLdt();
    // InitTss();
    // irq_init();

    // sti();
    // irq_unmask(IRQ_KEYBOARD);
}

void InitGdt(void)
{
    SegDesc *gdt = (SegDesc *) GDT_BASE;
    memset(gdt, 0, GDT_SIZE);

    MakeSegDesc(     // kernel code segment
        GetDescPtr(gdt, SEGSEL_KERNEL_CODE),
        DPL_KERNEL,
        0x0, LIMIT_MAX,
        DESC_MEM_CODE_XR);
    MakeSegDesc(     // kernel data segment
        GetDescPtr(gdt, SEGSEL_KERNEL_DATA),
        DPL_KERNEL,
        0x0, LIMIT_MAX,
        DESC_MEM_DATA_RW);
    MakeSegDesc(    // user data segment
        GetDescPtr(gdt, SEGSEL_USER_CODE),
        DPL_USER,
        0x0, LIMIT_MAX,
        DESC_MEM_CODE_XR);
    MakeSegDesc(    // user data segment
        GetDescPtr(gdt, SEGSEL_USER_DATA),
        DPL_USER,
        0x0, LIMIT_MAX,
        DESC_MEM_DATA_RW);
    MakeLdtDesc(    // LDT segment
        GetDescPtr(gdt, SEGSEL_LDT),
        DPL_KERNEL,
        LDT_BASE, LDT_SIZE - 1);
    MakeTssDesc(    // TSS segment
        GetDescPtr(gdt, SEGSEL_TSS),
        DPL_KERNEL,
        TSS_BASE, TSS_SIZE - 1);

    // TODO: stack segment?

// #define QW2DW(q) (uint32_t) ((q) >> 32), (uint32_t) (q)       // Qword to Dword

//     printf("boot: gdt: (%02x|%d)=%08x'%08x %s\n", SEGSEL_NULL & 0xFFF8, SEGSEL_NULL & 3, QW2DW(GetDescPtr(gdt, SEGSEL_NULL)->_value), "SEGSEL_NULL");
//     printf("boot: gdt: (%02x|%d)=%08x'%08x %s\n", SEGSEL_LDT & 0xFFF8, SEGSEL_LDT & 3, QW2DW(GetDescPtr(gdt, SEGSEL_LDT)->_value), "SEGSEL_LDT");
//     printf("boot: gdt: (%02x|%d)=%08x'%08x %s\n", SEGSEL_KERNEL_CODE & 0xFFF8, SEGSEL_KERNEL_CODE & 3, QW2DW(GetDescPtr(gdt, SEGSEL_KERNEL_CODE)->_value), "SEGSEL_KERNEL_CODE");
//     printf("boot: gdt: (%02x|%d)=%08x'%08x %s\n", SEGSEL_KERNEL_DATA & 0xFFF8, SEGSEL_KERNEL_DATA & 3, QW2DW(GetDescPtr(gdt, SEGSEL_KERNEL_DATA)->_value), "SEGSEL_KERNEL_DATA");
//     printf("boot: gdt: (%02x|%d)=%08x'%08x %s\n", SEGSEL_USER_CODE & 0xFFF8, SEGSEL_USER_CODE & 3, QW2DW(GetDescPtr(gdt, SEGSEL_USER_CODE)->_value), "SEGSEL_USER_CODE");
//     printf("boot: gdt: (%02x|%d)=%08x'%08x %s\n", SEGSEL_USER_DATA & 0xFFF8, SEGSEL_USER_DATA & 3, QW2DW(GetDescPtr(gdt, SEGSEL_USER_DATA)->_value), "SEGSEL_USER_DATA");
//     printf("boot: gdt: (%02x|%d)=%08x'%08x %s\n", SEGSEL_TSS & 0xFFF8, SEGSEL_TSS & 3, QW2DW(GetDescPtr(gdt, SEGSEL_TSS)->_value), "SEGSEL_TSS");

// #undef QW2DW

    // memset(&g_gdtDesc, 0, sizeof(DescReg));
    // g_gdtDesc.base = GDT_BASE;
    // g_gdtDesc.limit = GDT_SIZE - 1;

    volatile void *pDescReg = (void *) (((char *) &g_gdtDesc));
    printf("0x%08x\n", pDescReg);

    // lgdt(pDescReg);
    // LoadCs(SEGSEL_KERNEL_CODE);
    // LoadDs(SEGSEL_KERNEL_DATA);
    // LoadEs(SEGSEL_KERNEL_DATA);
    // LoadFs(0);
    // LoadGs(0);
    // LoadSs(SEGSEL_KERNEL_DATA);
}

// static const idt_thunk exception_thunks[NUM_EXCEPTION] =
// {
//     _thunk_exception_00h, _thunk_exception_01h, _thunk_exception_02h, _thunk_exception_03h,
//     _thunk_exception_04h, _thunk_exception_05h, _thunk_exception_06h, _thunk_exception_07h,
//     _thunk_exception_08h, _thunk_exception_09h, _thunk_exception_0ah, _thunk_exception_0bh,
//     _thunk_exception_0ch, _thunk_exception_0dh, _thunk_exception_0eh, _thunk_exception_0fh,
//     _thunk_exception_10h, _thunk_exception_11h, _thunk_exception_12h, _thunk_exception_13h,
//     _thunk_exception_14h, _thunk_exception_15h, _thunk_exception_16h, _thunk_exception_17h,
//     _thunk_exception_18h, _thunk_exception_19h, _thunk_exception_1ah, _thunk_exception_19h,
//     _thunk_exception_1ch, _thunk_exception_1ch, _thunk_exception_1eh, _thunk_exception_1fh
// };

// static const idt_thunk irq_thunks[NUM_IRQ] =
// {
//     _thunk_irq_00h, _thunk_irq_01h, _thunk_irq_02h, _thunk_irq_03h,
//     _thunk_irq_04h, _thunk_irq_05h, _thunk_irq_06h, _thunk_irq_07h,
//     _thunk_irq_08h, _thunk_irq_09h, _thunk_irq_0ah, _thunk_irq_0bh,
//     _thunk_irq_0ch, _thunk_irq_0dh, _thunk_irq_0eh, _thunk_irq_0fh
// };

void InitIdt(void)
{
    // struct x86_desc *idt;
    // struct x86_desc *desc;
    // int count;

    // idt = (struct x86_desc *) IDT_BASE;
    // memset(idt, 0, IDT_SIZE);

    // count = IDT_SIZE / sizeof(struct x86_desc);
    // for (int idx = 0, e_num = 0, i_num = 0; idx < count; idx++) {
    //     e_num = idx - INT_EXCEPTION;
    //     i_num = idx - INT_IRQ;
    //     desc = idt + idx;

    //     if (idx >= INT_EXCEPTION && e_num < NUM_EXCEPTION) {
    //         set_trap_desc(desc, SEGSEL_KERNEL_CODE, DPL_KERNEL, exception_thunks[e_num]);
    //     }
    //     else if (idx >= INT_IRQ && i_num < NUM_IRQ) {
    //         set_intr_desc(desc, SEGSEL_KERNEL_CODE, DPL_KERNEL, irq_thunks[i_num]);
    //     }
    //     else if (idx == INT_SYSCALL) {
    //         set_trap_desc(desc, SEGSEL_KERNEL_CODE, DPL_USER, _thunk_syscall);
    //     }
    // }

    // g_idtdesc.base = IDT_BASE;
    // g_idtdesc.limit = IDT_SIZE - 1;
    // lidt(g_idtdesc);
}

void InitLdt(void)
{
    memset(g_ldt, 0, LDT_SIZE);

    lldt(SEGSEL_LDT);
}

void InitTss(void)
{
    memset(g_tss, 0, TSS_SIZE);

    g_tss->ldtSegSel = SEGSEL_LDT;
    g_tss->esp0 = KERNEL_STACK;
    g_tss->ss0 = SEGSEL_KERNEL_DATA;

    ltr(SEGSEL_TSS);
}
