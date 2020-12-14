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

#include <nb/boot.h>
#include <nb/init.h>
#include <nb/kernel.h>
#include <nb/console.h>
#include <nb/x86_desc.h>

#include <drivers/vga.h>

void kmain(void)
{
    gdt_init();
    ldt_init();
    idt_init();
    tss_init();
    con_init();

    char *framebuf = (char *) VGA_FRAMEBUF_COLOR;
    framebuf[160] = '!';
    framebuf[161] = 2;
    framebuf[162] = '?';
    framebuf[163] = 5;

    uint16_t pos = get_cursor_pos();
    framebuf[pos * 2] = 'X';
    set_cursor_pos(++pos);
    set_cursor_shape(0, 15);
}

void gdt_init(void)
{
    segdesc_t *gdt = (segdesc_t *) GDT_BASE;
    memset(gdt, 0, GDT_SIZE);

    set_segdesc(gdt, KERNEL_CS, 0, 0xFFFFF, SEGDESC_TYPE_XR, 0);
    set_segdesc(gdt, KERNEL_DS, 0, 0xFFFFF, SEGDESC_TYPE_RW, 0);
    set_segdesc(gdt, USER_CS, 0, 0xFFFFF, SEGDESC_TYPE_XR, 3);
    set_segdesc(gdt, USER_DS, 0, 0xFFFFF, SEGDESC_TYPE_RW, 3);
    set_segdesc_sys(gdt, LDT_SEG, LDT_BASE, LDT_SIZE-1, SEGDESC_TYPE_LDT);
    set_segdesc_tss(gdt, TSS_SEG, TSS_BASE, TSS_SIZE-1);

    descreg_t *gdtr = (descreg_t *) GDT_REGPTR;
    gdtr->base = GDT_BASE;
    gdtr->limit = GDT_SIZE - 1;
    lgdt(*gdtr);

    load_cs(KERNEL_CS);
    load_ds(KERNEL_DS);
    load_ss(KERNEL_DS);
    load_es(NULL);
    load_fs(NULL);
    load_gs(NULL);
}

void ldt_init(void)
{
    segdesc_t *ldt = (segdesc_t *) LDT_BASE;
    memset(ldt, 0, LDT_SIZE);   /* not using the LDT, so just zero it */

    lldt(LDT_SEG);
}

void idt_init(void)
{

}

void tss_init(void)
{
    struct tss *tss = (struct tss *) TSS_BASE;

    tss->ldt_segsel = LDT_SEG;
    tss->esp0 = KERN_STACK;
    tss->ss0 = KERNEL_DS;
    ltr(TSS_SEG);
}
