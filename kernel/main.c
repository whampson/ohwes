/*============================================================================*
 * Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                  *
 *                                                                            *
 * This file is part of the OHWES Operating System.                           *
 * OHWES is free software; you may redistribute it and/or modify it under the *
 * terms of the license agreement provided with this software.                *
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

#include <ctype.h>
#include <stdlib.h>
#include <ohwes/boot.h>
#include <ohwes/init.h>
#include <ohwes/kernel.h>
#include <ohwes/irq.h>
#include <ohwes/interrupt.h>
#include <ohwes/io.h>
#include <ohwes/keyboard.h>
#include <ohwes/test.h>
#include <x86/desc.h>
#include <drivers/vga.h>

static bool keydown(vk_t key)
{
    return kbd_ioctl(KBKEYDOWN, key);
}

void kmain(void)
{
    gdt_init();
    ldt_init();
    idt_init();
    tss_init();
    irq_init();
    con_init();
    kbd_init();
    mem_init();
    sti();

    while (1) {
        if ((keydown(VK_LCTRL) || keydown(VK_RCTRL)) && keydown(VK_F8)) {
            start_interactive_tests();
        }
    }
}

void gdt_init(void)
{
    struct x86_desc *gdt;
    struct x86_desc *kcs, *kds, *ucs, *uds, *ldt, *tss;

    gdt = (struct x86_desc *) GDT_BASE;
    memset(gdt, 0, GDT_SIZE);

    kcs = get_seg_desc(gdt, KERNEL_CS);
    kds = get_seg_desc(gdt, KERNEL_DS);
    ucs = get_seg_desc(gdt, USER_CS);
    uds = get_seg_desc(gdt, USER_DS);
    ldt = get_seg_desc(gdt, LDT);
    tss = get_seg_desc(gdt, TSS);

    set_seg_desc(kcs, KERNEL_PL, 1, 0x00000000, LIMIT_MAX, 1, DESC_TYPE_CODE_XR);
    set_seg_desc(kds, KERNEL_PL, 1, 0x00000000, LIMIT_MAX, 1, DESC_TYPE_DATA_RW);
    set_seg_desc(ucs, USER_PL,   1, 0x00000000, LIMIT_MAX, 1, DESC_TYPE_CODE_XR);
    set_seg_desc(uds, USER_PL,   1, 0x00000000, LIMIT_MAX, 1, DESC_TYPE_DATA_RW);
    set_ldt_desc(ldt, KERNEL_PL, 1, LDT_BASE,   LDT_SIZE-1,0);
    set_tss_desc(tss, KERNEL_PL, 1, TSS_BASE,   TSS_SIZE-1,0);
    /* TODO: stack segment? */

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
    memset(ldt, 0, LDT_SIZE);

    /* we're not using LDTs, so just load an empty one */
    lldt(LDT);
}

void tss_init(void)
{
    struct tss *tss = (struct tss *) TSS_BASE;
    tss->ldt_segsel = LDT;
    tss->esp0 = KERNEL_STACK;
    tss->ss0 = KERNEL_DS;

    /* we're also not really using the TSS, so just set up the minimum
       parameters to keep the CPU happy :) */
    ltr(TSS);
}
