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

// #include "init.h"
#include "debug.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// #include <console.h>
#include <sys/boot.h>
#include <sys/io.h>
#include <sys/interrupt.h>
#include <sys/os.h>

static BootParams LocalBootParams = { };
BootParams *g_pBootParams = &LocalBootParams;

extern void IrqInit(void);
static void InitCpuDesc(void);

static const IdtThunk ExceptionThunks[NUM_EXCEPTION] =
{
    Exception00h, Exception01h, Exception02h, Exception03h,
    Exception04h, Exception05h, Exception06h, Exception07h,
    Exception08h, Exception09h, Exception0Ah, Exception0Bh,
    Exception0Ch, Exception0Dh, Exception0Eh, Exception0Fh,
    Exception10h, Exception11h, Exception12h, Exception13h,
    Exception14h, Exception15h, Exception16h, Exception17h,
    Exception18h, Exception19h, Exception1Ah, Exception19h,
    Exception1Ch, Exception1Ch, Exception1Eh, Exception1Fh
};

static const IdtThunk IrqThunks[NUM_IRQ] =
{
    Irq00h, Irq01h, Irq02h, Irq03h,
    Irq04h, Irq05h, Irq06h, Irq07h,
    Irq08h, Irq09h, Irq0Ah, Irq0Bh,
    Irq0Ch, Irq0Dh, Irq0Eh, Irq0Fh
};

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

__fastcall
void KeMain(const BootParams *pParams)
{
    memcpy(g_pBootParams, pParams, sizeof(BootParams));

    printf("\n");
    PrintHardwareInfo();
    PrintMemoryInfo();

    InitCpuDesc();  // TODO: something in here overwrite mem map...

    IrqInit();
    IrqUnmask(IRQ_KEYBOARD);
    sti();
}

static void InitCpuDesc(void)
{
    //
    // Global Descriptor Table
    //
    SegDesc *gdt = (SegDesc *) GDT_BASE;
    memset(gdt, 0, GDT_LIMIT+1);

    MakeSegDesc(
        GetDescPtr(gdt, SEGSEL_KERNEL_CODE),
        KernelMode,
        0x0, LIMIT_MAX,
        DESC_MEM_CODE_XR);
    MakeSegDesc(
        GetDescPtr(gdt, SEGSEL_KERNEL_DATA),
        KernelMode,
        0x0, LIMIT_MAX,
        DESC_MEM_DATA_RW);
    MakeSegDesc(
        GetDescPtr(gdt, SEGSEL_USER_CODE),
        UserMode,
        0x0, LIMIT_MAX,
        DESC_MEM_CODE_XR);
    MakeSegDesc(
        GetDescPtr(gdt, SEGSEL_USER_DATA),
        UserMode,
        0x0, LIMIT_MAX,
        DESC_MEM_DATA_RW);
    MakeLdtDesc(
        GetDescPtr(gdt, SEGSEL_LDT),
        KernelMode,
        LDT_BASE, LDT_SIZE - 1);
    MakeTssDesc(
        GetDescPtr(gdt, SEGSEL_TSS),
        KernelMode,
        TSS_BASE, TSS_SIZE - 1);

    lgdt(g_gdtDesc);
    LoadCs(SEGSEL_KERNEL_CODE);
    LoadDs(SEGSEL_KERNEL_DATA);
    LoadEs(SEGSEL_KERNEL_DATA);
    LoadFs(0);
    LoadGs(0);
    LoadSs(SEGSEL_KERNEL_DATA);

    //
    // Interrupt Descriptor Table
    //
    int count = IDT_SIZE / sizeof(SegDesc);
    for (int idx = 0, e = 0, i = 0; idx < count; idx++) {
        e = idx - INT_EXCEPTION;
        i = idx - INT_IRQ;
        SegDesc *desc = g_idt + idx;

        if (idx >= INT_EXCEPTION && e < NUM_EXCEPTION) {
            MakeTrapDesc(desc, SEGSEL_KERNEL_CODE, KernelMode, ExceptionThunks[e]);
        }
        else if (idx >= INT_IRQ && i < NUM_IRQ) {
            MakeIntrDesc(desc, SEGSEL_KERNEL_CODE, KernelMode, IrqThunks[i]);
        }
        else if (idx == INT_SYSCALL) {
            MakeTrapDesc(desc, SEGSEL_KERNEL_CODE, UserMode, Syscall);
        }
    }
    lidt(g_idtDesc);

    //
    // Local Descriptor Table
    //
    memset(g_ldt, 0, LDT_SIZE);
    lldt(SEGSEL_LDT);

    //
    // TSS
    //
    memset(g_tss, 0, TSS_SIZE);
    g_tss->ldtSegSel = SEGSEL_LDT;
    g_tss->esp0 = (uint32_t) g_pBootParams->m_pEbda;
    g_tss->ss0 = SEGSEL_KERNEL_DATA;
    ltr(SEGSEL_TSS);
}
