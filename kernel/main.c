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

#include <string.h>
#include <nb/init.h>
#include <nb/niobium.h>
#include <nb/x86_desc.h>

static void gdt_init(void);
static void ldt_init(void);
static void idt_init(void);
static void tss_init(void);

void kmain(void)
{
    gdt_init();
    ldt_init();
    idt_init();
    tss_init();


    char *vid_mem = (char *) 0xb8000;
    vid_mem[160]++;
    vid_mem[161] = 2;

    // for (;;);
}

static void gdt_init(void)
{
    segdesc_t *gdt = (segdesc_t *) GDT_BASE;
    memset(gdt, 0, GDT_SIZE);

    set_segdesc(gdt, KERNEL_CS, 0, 0xFFFFF, SEGDESC_TYPE_XR, 0);
    set_segdesc(gdt, KERNEL_DS, 0, 0xFFFFF, SEGDESC_TYPE_RW, 0);
    set_segdesc(gdt, USER_CS, 3, 0xFFFFF, SEGDESC_TYPE_XR, 0);
    set_segdesc(gdt, USER_DS, 3, 0xFFFFF, SEGDESC_TYPE_RW, 0);
    // set_segdesc(gdt, LDT_SEG, 0, 0, SEGDESC_TYPE_LDT, 0);
    // set_segdesc(gdt, TSS_SEG, 0, 0, SEGDESC_TYPE_TSS32, 0);

    descreg_t *gdtr = (descreg_t *) GDT_PTR;
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

static void ldt_init(void)
{

}

static void idt_init(void)
{

}

static void tss_init(void)
{

}
