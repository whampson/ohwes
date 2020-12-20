/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: kernel/main.c                                                     *
 * Created: December 9, 2020                                                  *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <nb/boot.h>
#include <nb/init.h>
#include <nb/kernel.h>
#include <nb/console.h>
#include <nb/memory.h>
#include <x86/desc.h>
#include <x86/cntrl.h>

#include <drivers/vga.h>

void kmain(void)
{
    gdt_init();
    ldt_init();
    idt_init();
    tss_init();
    con_init();
    printk("\033[H\033[2JOHWES\n\n");
    mem_init();

    uint8_t grfx_misc = vga_grfx_read(VGA_REG_GRFX_MISC);
    grfx_misc &= ~VGA_FLD_GRFX_MISC_MMAP;
    grfx_misc |= (VGA_ENUM_GRFX_MISC_MMAP_32K_LO) << 2;
    vga_grfx_write(VGA_REG_GRFX_MISC, grfx_misc);

    char *test = (char *) 0xB0000;

    for (int i = 0; i < 15; i++) {
        test[0] = 'a' + i;
        test += 0x1000;
    }

    printk("\033[41mThe quick brown fox jumps over the lazy dog\033[0m\n");
}

void gdt_init(void)
{
    struct segdesc *gdt = (struct segdesc *) GDT_BASE;
    memset(gdt, 0, GDT_SIZE);

    set_segdesc(gdt, KERNEL_CS, 0, 0xFFFFF, SEGDESC_TYPE_XR, 0);
    set_segdesc(gdt, KERNEL_DS, 0, 0xFFFFF, SEGDESC_TYPE_RW, 0);
    set_segdesc(gdt, USER_CS, 0, 0xFFFFF, SEGDESC_TYPE_XR, 3);
    set_segdesc(gdt, USER_DS, 0, 0xFFFFF, SEGDESC_TYPE_RW, 3);
    set_segdesc_sys(gdt, LDT_SEG, LDT_BASE, LDT_SIZE-1, SEGDESC_TYPE_LDT);
    set_segdesc_tss(gdt, TSS_SEG, TSS_BASE, TSS_SIZE-1);

    struct descreg *gdtr = (struct descreg *) GDT_REGPTR;
    gdtr->base = GDT_BASE;
    gdtr->limit = GDT_SIZE - 1;
    lgdt(*gdtr);

    load_cs(KERNEL_CS);
    load_ds(KERNEL_DS);
    load_ss(KERNEL_DS);
    load_es(KERNEL_DS);
    load_fs(NULL);
    load_gs(NULL);
}

void ldt_init(void)
{
    struct segdesc *ldt = (struct segdesc *) LDT_BASE;
    memset(ldt, 0, LDT_SIZE);   /* not using the LDT, so just zero it */
    lldt(LDT_SEG);
}

void tss_init(void)
{
    struct tss *tss = (struct tss *) TSS_BASE;
    tss->ldt_segsel = LDT_SEG;
    tss->esp0 = KERN_STACK;
    tss->ss0 = KERNEL_DS;
    ltr(TSS_SEG);
}
