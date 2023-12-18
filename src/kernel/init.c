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
 *         File: kernel/init.c
 *      Created: March 26, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#include "init.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// #include <console.h>
#include <boot.h>
#include <io.h>
#include <interrupt.h>
#include <os.h>
#include <x86.h>

extern void con_init(void);
// extern void IrqInit(void);
// static void InitCpuDesc(void);

// static const IdtThunk ExceptionThunks[NUM_EXCEPTION] =
// {
//     Exception00h, Exception01h, Exception02h, Exception03h,
//     Exception04h, Exception05h, Exception06h, Exception07h,
//     Exception08h, Exception09h, Exception0Ah, Exception0Bh,
//     Exception0Ch, Exception0Dh, Exception0Eh, Exception0Fh,
//     Exception10h, Exception11h, Exception12h, Exception13h,
//     Exception14h, Exception15h, Exception16h, Exception17h,
//     Exception18h, Exception19h, Exception1Ah, Exception19h,
//     Exception1Ch, Exception1Ch, Exception1Eh, Exception1Fh
// };

// static const IdtThunk IrqThunks[NUM_IRQ] =
// {
//     Irq00h, Irq01h, Irq02h, Irq03h,
//     Irq04h, Irq05h, Irq06h, Irq07h,
//     Irq08h, Irq09h, Irq0Ah, Irq0Bh,
//     Irq0Ch, Irq0Dh, Irq0Eh, Irq0Fh
// };

/* Segment Selectors */
#define SEGSEL_NULL         (0x0)
#define SEGSEL_LDT          (0x08|KernelMode)
#define SEGSEL_KERNEL_CODE  (0x10|KernelMode)
#define SEGSEL_KERNEL_DATA  (0x18|KernelMode)
#define SEGSEL_USER_CODE    (0x20|UserMode)
#define SEGSEL_USER_DATA    (0x28|UserMode)
#define SEGSEL_TSS          (0x30|KernelMode)

SegDesc * const g_gdt = (SegDesc *) GDT_BASE;
SegDesc * const g_idt = (SegDesc *) IDT_BASE;
SegDesc * const g_ldt = (SegDesc *) LDT_BASE;
struct Tss * const g_tss = (struct Tss *) TSS_BASE;

__align(2) DescReg g_gdtDesc = { GDT_LIMIT, GDT_BASE };
__align(2) DescReg g_idtDesc = { IDT_LIMIT, IDT_BASE };

#define OS_NAME_STRING      "OHWES"
#define OS_VERSION_STRING   "0.1"
#define OS_COPYRIGHT_STRING "(C) 2020-2023 Wes Hampson. All Rights Reserved."

#define BootPrint(fmt,...)  printf("boot: " fmt, __VA_ARGS__)

__fastcall
void KeMain(const BootParams * const pBootInfo)
{
    con_init();

    // InitCpuDesc();  // TODO: something in here crashes...
    // IrqInit();
    // IrqUnmask(IRQ_KEYBOARD);
    // sti();

    printf("Hello, world!\n");

    {
        #define PrintParam(x) BootPrint(#x " = 0x%x\n", x);

        PrintParam(pBootInfo->m_HwFlags.HasDisketteDrive);
        PrintParam(pBootInfo->m_HwFlags.HasCoprocessor);
        PrintParam(pBootInfo->m_HwFlags.HasPs2Mouse);
        PrintParam(pBootInfo->m_HwFlags.VideoMode);
        PrintParam(pBootInfo->m_HwFlags.NumOtherDisketteDrives);
        // // PrintParam(pBootInfo->m_HwFlags._Dma);
        PrintParam(pBootInfo->m_HwFlags.NumSerialPorts);
        PrintParam(pBootInfo->m_HwFlags.HasGamePort);
        // PrintParam(pBootInfo->m_HwFlags._HasPrinterOrModem);
        PrintParam(pBootInfo->m_HwFlags.NumParallelPorts);
        // PrintParam(pBootInfo->m_A20Method);
        // PrintParam(pBootInfo->m_VideoMode);
        // PrintParam(pBootInfo->m_VideoPage);
        // PrintParam(pBootInfo->m_VideoCols);
        // PrintParam(pBootInfo->m_CursorStartLine);
        // PrintParam(pBootInfo->m_CursorEndLine);

        PrintParam(pBootInfo->m_KernelBase);
        PrintParam(pBootInfo->m_KernelSize);
        PrintParam(pBootInfo->m_Stage2Base);
        PrintParam(pBootInfo->m_Stage2Size);
        PrintParam(pBootInfo->m_StackBase);
        PrintParam(pBootInfo->m_RamLo_Legacy);
        PrintParam(pBootInfo->m_RamHi_Legacy);
        PrintParam(pBootInfo->m_RamLo_E801h);
        PrintParam(pBootInfo->m_RamHi_E801h << 6);
        PrintParam(pBootInfo->m_pMemoryMap);
        PrintParam(pBootInfo->m_pEbda);

        #undef PrintParam

        const AcpiMemoryMapEntry *memMap = pBootInfo->m_pMemoryMap;
        if (pBootInfo->m_pMemoryMap) {
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


}

// static void InitCpuDesc(void)
// {
//     //
//     // Global Descriptor Table
//     //
//     SegDesc *gdt = (SegDesc *) GDT_BASE;
//     memset(gdt, 0, GDT_SIZE);

//     MakeSegDesc(
//         GetDescPtr(gdt, SEGSEL_KERNEL_CODE),
//         KernelMode,
//         0x0, LIMIT_MAX,
//         DESC_MEM_CODE_XR);
//     MakeSegDesc(
//         GetDescPtr(gdt, SEGSEL_KERNEL_DATA),
//         KernelMode,
//         0x0, LIMIT_MAX,
//         DESC_MEM_DATA_RW);
//     MakeSegDesc(
//         GetDescPtr(gdt, SEGSEL_USER_CODE),
//         UserMode,
//         0x0, LIMIT_MAX,
//         DESC_MEM_CODE_XR);
//     MakeSegDesc(
//         GetDescPtr(gdt, SEGSEL_USER_DATA),
//         UserMode,
//         0x0, LIMIT_MAX,
//         DESC_MEM_DATA_RW);
//     MakeLdtDesc(
//         GetDescPtr(gdt, SEGSEL_LDT),
//         KernelMode,
//         LDT_BASE, LDT_SIZE - 1);
//     MakeTssDesc(
//         GetDescPtr(gdt, SEGSEL_TSS),
//         KernelMode,
//         TSS_BASE, TSS_SIZE - 1);

//     lgdt(g_gdtDesc);
//     LoadCs(SEGSEL_KERNEL_CODE);
//     LoadDs(SEGSEL_KERNEL_DATA);
//     LoadEs(SEGSEL_KERNEL_DATA);
//     LoadFs(0);
//     LoadGs(0);
//     LoadSs(SEGSEL_KERNEL_DATA);

//     //
//     // Interrupt Descriptor Table
//     //
//     int count = IDT_SIZE / sizeof(SegDesc);
//     for (int idx = 0, e = 0, i = 0; idx < count; idx++) {
//         e = idx - INT_EXCEPTION;
//         i = idx - INT_IRQ;
//         SegDesc *desc = g_idt + idx;

//         if (idx >= INT_EXCEPTION && e < NUM_EXCEPTION) {
//             MakeTrapDesc(desc, SEGSEL_KERNEL_CODE, KernelMode, ExceptionThunks[e]);
//         }
//         else if (idx >= INT_IRQ && i < NUM_IRQ) {
//             MakeIntrDesc(desc, SEGSEL_KERNEL_CODE, KernelMode, IrqThunks[i]);
//         }
//         else if (idx == INT_SYSCALL) {
//             MakeTrapDesc(desc, SEGSEL_KERNEL_CODE, UserMode, Syscall);
//         }
//     }
//     lidt(g_idtDesc);

//     //
//     // Local Descriptor Table
//     //
//     memset(g_ldt, 0, LDT_SIZE);
//     lldt(SEGSEL_LDT);

//     //
//     // TSS
//     //
//     memset(g_tss, 0, TSS_SIZE);
//     g_tss->ldtSegSel = SEGSEL_LDT;
//     g_tss->esp0 = (uint32_t) g_BootParams->m_p;
//     g_tss->ss0 = SEGSEL_KERNEL_DATA;
//     ltr(SEGSEL_TSS);
// }
