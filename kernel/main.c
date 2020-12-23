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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ohwes/ohwes.h>
#include <ohwes/boot.h>
#include <ohwes/init.h>
#include <ohwes/kernel.h>
#include <ohwes/console.h>
#include <ohwes/memory.h>
#include <ohwes/interrupt.h>
#include <ohwes/syscall.h>
#include <ohwes/types.h>
#include <x86/desc.h>
#include <x86/cntrl.h>

int test0(void)
{
    __syscall_setup;
    __syscall0(SYS_TEST0);
    __syscall_ret;
}

int test1(int a)
{
    __syscall_setup;
    __syscall1(SYS_TEST1, a);
    __syscall_ret;
}

int test2(int a, int b)
{
    __syscall_setup;
    __syscall2(SYS_TEST2, a, b);
    __syscall_ret;
}
int test3(int a, char b, int c)
{
    __syscall_setup;
    __syscall3(SYS_TEST3, a, b, c);
    __syscall_ret;
}
int test4(int a, int b, int c, int d)
{
    __syscall_setup;
    __syscall4(SYS_TEST4, a, b, c, d);
    __syscall_ret;
}
int test5(int a, int b, int c, int d, int e)
{
    __syscall_setup;
    __syscall5(SYS_TEST5, a, b, c, d, e);
    __syscall_ret;
}

int sys_test0(void)
{
    kprintf("sys_test0()\n");
    return 0;
}

int sys_test1(int a)
{
    kprintf("sys_test1(): a=%p\n", a);
    return 0;
}

int sys_test2(int a, int b)
{
    kprintf("sys_test2(): a=%p, b=%p\n", a,b);
    return 0;
}

int sys_test3(int a, char b, int c)
{
    kprintf("sys_test3(): a=%p, b=%p, c=%p\n", a,b,c);
    return 0;
}

int sys_test4(int a, int b, int c, int d)
{
    kprintf("sys_test4(): a=%p, b=%p, c=%p, d=%p\n", a,b,c,d);
    return 0;
}

int sys_test5(int a, int b, int c, int d, int e)
{
    kprintf("sys_test5(): a=%p, b=%p, c=%p, d=%p, e=%p\n", a,b,c,d,e);
    return 0;
}

void kmain(void)
{
    gdt_init();
    ldt_init();
    idt_init();
    tss_init();
    con_init();

    kprintf("Into TheKernel!!!\n\n");

    mem_init();

    kprintf("\nOHWES 0.1\n");
    kprintf("Copyright (C) 2020-2021 Wes Hampson\n\n");


    test0();
    test1(0xFFFFFF01);
    test2(0xFFFFFF02, 0xFFFFFF03);
    test3(0xFFFFFF03, 0x04, 0xFFFFFF05);
    test4(0xFFFFFF04, 0xFFFFFF05, 0xFFFFFF06, 0xFFFFFF07);
    test5(0xFFFFFF05, 0xFFFFFF06, 0xFFFFFF07, 0xFFFFFF08, 0xFFFFFF09);
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
    tss->esp0 = KERN_STACK;
    tss->ss0 = KERNEL_DS;

    /* we're also not really using the TSS, so just set up the minimum
       parameters to keep the CPU happy :) */
    ltr(TSS);
}
