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
 * =============================================================================
 */

#include "boot.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <os/console.h>
#include <hw/x86_desc.h>

/* Privilege Levels */
#define KERNEL_PL       0                       /* Kernel Privilege Level */
#define USER_PL         3                       /* User Privilege Level */

/* Segment Selectors */
#define LDT_SEG         (0x08|KERNEL_PL)        /* LDT Segment */
#define KCS_SEG         (0x10|KERNEL_PL)        /* Kernel Code Segment */
#define KDS_SEG         (0x18|KERNEL_PL)        /* Kernel Data Segment */
#define UCS_SEG         (0x20|USER_PL)          /* User Code Segment */
#define UDS_SEG         (0x28|USER_PL)          /* User Data Segment */
#define TSS_SEG         (0x30|KERNEL_PL)        /* TSS Segment */

#define KERNEL_STACK    0x7C00

__attribute__ ((aligned)) struct desc_reg g_gdtdesc = { 0 };
__attribute__ ((aligned)) struct desc_reg g_idtdesc = { 0 };

void InitGdt();
void InitIdt();
void InitLdt();
void InitTss();
void irq_init();

void init()
{
    cli();

    const HwFlags *hwFlags = (HwFlags *) &g_HwFlags;
    printf("hwflags: diskette drive? %s\n", hwFlags->HasDisketteDrive ? "yes" : "no");
    printf("hwflags: coprocessor? %s\n", hwFlags->HasCoprocessor ? "yes" : "no");
    printf("hwflags: PS/2 mouse? %s\n", hwFlags->HasPs2Mouse ? "yes" : "no");
    printf("hwflags: game port? %s\n", hwFlags->HasGamePort ? "yes" : "no");
    printf("hwflags: num serial ports = %d\n", hwFlags->NumSerialPorts);
    printf("hwflags: num parallel ports = %d\n", hwFlags->NumParallelPorts);
    printf("hwflags: num secondary diskette drives = %d\n", hwFlags->NumOtherDisketteDrives);
    printf("hwflags: video mode = ");
    switch (hwFlags->VideoMode) {
        case HWFLAGS_VIDEOMODE_40x25:       printf("40x25\n"); break;
        case HWFLAGS_VIDEOMODE_80x25:       printf("80x25\n"); break;
        case HWFLAGS_VIDEOMODE_80x25_MONO:  printf("80x25 (monochrome)\n"); break;
        default:                            printf("(invalid)\n"); break;
    }

    printf("A20: ");
    switch (g_A20Method) {
        case A20METHOD_NONE:        printf("enabled\n"); break;
        case A20METHOD_KEYBOARD:    printf("enabled via PS/2 keyboard controller\n"); break;
        case A20METHOD_PORT92h:     printf("enabled via I/O port 92h\n"); break;
        case A20METHOD_BIOS:        printf("enabled via BIOS INT=15h,AX=2401h\n"); break;
        default:                    printf("(invalid)"); break;
    }

    printf("RAM: g_RamLo_Legacy = %d\n", g_RamLo_Legacy);
    printf("RAM: g_RamHi_Legacy = %d\n", g_RamHi_Legacy);
    printf("RAM: g_RamLo_E801h = %d\n", g_RamLo_E801h);
    printf("RAM: g_RamHi_E801h = %d\n", g_RamHi_E801h << 6);    // 64K pages to 1K pages

    if (g_HasAcpiMemoryMap) {
        const AcpiMemoryMapEntry *memMap = g_AcpiMemoryMap;
        do {
            if (memMap->Length > 0) {
                printf("RAM: BIOS-E820h: %08x-%08x ",
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

    InitIdt();
    InitGdt();
    InitLdt();
    InitTss();
    irq_init();

    sti();
    irq_unmask(IRQ_KEYBOARD);
}


void InitGdt(void)
{
    struct x86_desc *gdt;
    struct x86_desc *kcs, *kds, *ucs, *uds, *ldt, *tss;

    gdt = (struct x86_desc *) GDT_BASE;
    memset(gdt, 0, GDT_SIZE);

    kcs = get_seg_desc(gdt, KCS_SEG);
    kds = get_seg_desc(gdt, KDS_SEG);
    ucs = get_seg_desc(gdt, UCS_SEG);
    uds = get_seg_desc(gdt, UDS_SEG);
    ldt = get_seg_desc(gdt, LDT_SEG);
    tss = get_seg_desc(gdt, TSS_SEG);

    set_seg_desc(kcs, KERNEL_PL, 1, 0x00000000, LIMIT_MAX, 1, DESC_TYPE_CODE_XR);
    set_seg_desc(kds, KERNEL_PL, 1, 0x00000000, LIMIT_MAX, 1, DESC_TYPE_DATA_RW);
    set_seg_desc(ucs, USER_PL,   1, 0x00000000, LIMIT_MAX, 1, DESC_TYPE_CODE_XR);
    set_seg_desc(uds, USER_PL,   1, 0x00000000, LIMIT_MAX, 1, DESC_TYPE_DATA_RW);
    set_ldt_desc(ldt, KERNEL_PL, 1, LDT_BASE,   LDT_SIZE-1,0);
    set_tss_desc(tss, KERNEL_PL, 1, TSS_BASE,   TSS_SIZE-1,0);
    /* TODO: stack segment? */

// #define QW2DW(q)    (uint32_t) ((q) >> 32), (uint32_t) (q)

//     printf("gdt: (%02x|%d)=%08x'%08x %s\n", LDT_SEG & 0xFFF8, LDT_SEG & 3, QW2DW(ldt->_value), "LDT_SEG");
//     printf("gdt: (%02x|%d)=%08x'%08x %s\n", KCS_SEG & 0xFFF8, KCS_SEG & 3, QW2DW(kcs->_value), "KCS_SEG");
//     printf("gdt: (%02x|%d)=%08x'%08x %s\n", KDS_SEG & 0xFFF8, KDS_SEG & 3, QW2DW(kds->_value), "KDS_SEG");
//     printf("gdt: (%02x|%d)=%08x'%08x %s\n", UCS_SEG & 0xFFF8, UCS_SEG & 3, QW2DW(ucs->_value), "UCS_SEG");
//     printf("gdt: (%02x|%d)=%08x'%08x %s\n", UDS_SEG & 0xFFF8, UDS_SEG & 3, QW2DW(uds->_value), "UDS_SEG");
//     printf("gdt: (%02x|%d)=%08x'%08x %s\n", TSS_SEG & 0xFFF8, TSS_SEG & 3, QW2DW(tss->_value), "TSS_SEG");

    g_gdtdesc.base = GDT_BASE;
    g_gdtdesc.limit = GDT_SIZE - 1;

    lgdt(g_gdtdesc);
    load_cs(KCS_SEG);
    load_ds(KDS_SEG);
    load_es(KDS_SEG);
    load_fs(0);
    load_gs(0);
    load_ss(KDS_SEG);
}

static const idt_thunk exception_thunks[NUM_EXCEPTION] =
{
    _thunk_exception_00h, _thunk_exception_01h, _thunk_exception_02h, _thunk_exception_03h,
    _thunk_exception_04h, _thunk_exception_05h, _thunk_exception_06h, _thunk_exception_07h,
    _thunk_exception_08h, _thunk_exception_09h, _thunk_exception_0ah, _thunk_exception_0bh,
    _thunk_exception_0ch, _thunk_exception_0dh, _thunk_exception_0eh, _thunk_exception_0fh,
    _thunk_exception_10h, _thunk_exception_11h, _thunk_exception_12h, _thunk_exception_13h,
    _thunk_exception_14h, _thunk_exception_15h, _thunk_exception_16h, _thunk_exception_17h,
    _thunk_exception_18h, _thunk_exception_19h, _thunk_exception_1ah, _thunk_exception_19h,
    _thunk_exception_1ch, _thunk_exception_1ch, _thunk_exception_1eh, _thunk_exception_1fh
};

static const idt_thunk irq_thunks[NUM_IRQ] =
{
    _thunk_irq_00h, _thunk_irq_01h, _thunk_irq_02h, _thunk_irq_03h,
    _thunk_irq_04h, _thunk_irq_05h, _thunk_irq_06h, _thunk_irq_07h,
    _thunk_irq_08h, _thunk_irq_09h, _thunk_irq_0ah, _thunk_irq_0bh,
    _thunk_irq_0ch, _thunk_irq_0dh, _thunk_irq_0eh, _thunk_irq_0fh
};

void InitIdt(void)
{
    struct x86_desc *idt;
    struct x86_desc *desc;
    int count;

    idt = (struct x86_desc *) IDT_BASE;
    memset(idt, 0, IDT_SIZE);

    count = IDT_SIZE / sizeof(struct x86_desc);
    for (int idx = 0, e_num = 0, i_num = 0; idx < count; idx++) {
        e_num = idx - INT_EXCEPTION;
        i_num = idx - INT_IRQ;
        desc = idt + idx;

        if (idx >= INT_EXCEPTION && e_num < NUM_EXCEPTION) {
            set_trap_desc(desc, KCS_SEG, KERNEL_PL, exception_thunks[e_num]);
        }
        else if (idx >= INT_IRQ && i_num < NUM_IRQ) {
            set_intr_desc(desc, KCS_SEG, KERNEL_PL, irq_thunks[i_num]);
        }
        else if (idx == INT_SYSCALL) {
            set_trap_desc(desc, KCS_SEG, USER_PL, _thunk_syscall);
        }
    }

    g_idtdesc.base = IDT_BASE;
    g_idtdesc.limit = IDT_SIZE - 1;
    lidt(g_idtdesc);
}

void InitLdt(void)
{
    struct segdesc *ldt = (struct segdesc *) LDT_BASE;
    memset(ldt, 0, LDT_SIZE);

    lldt(LDT_SEG);
}

void InitTss(void)
{
    struct tss *tss = (struct tss *) TSS_BASE;
    memset(tss, 0, TSS_SIZE);

    tss->ldt_seg = LDT_SEG;
    tss->esp0 = KERNEL_STACK;
    tss->ss0 = KDS_SEG;

    ltr(TSS_SEG);
}
